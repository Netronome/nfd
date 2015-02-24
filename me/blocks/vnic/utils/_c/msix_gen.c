#include <assert.h>
#include <vnic/shared/nfcc_chipres.h>
#include <nfp.h>

#include <nfp/me.h>
#include <nfp/mem_bulk.h>

#include <nfp6000/nfp_me.h>
#include <stdint.h>
#include <types.h>

#include <msix.h>
#include <vnic/shared/nfd_cfg.h>
#include <vnic/pci_out.h>
#include <vnic/shared/nfd_internal.h>
#include <nfp/mem_atomic.h>
#include <std/reg_utils.h>


/*
  TODO:
  - need to clear all pending interrupts when function comes up or down (what if the state didn't change? save state?)
  - any other operation when link comes down
  - read counters in bulk
  - race condition when sending interrupt
*/

#define MAX_QUEUE_NUM (NFD_MAX_VFS*NFD_MAX_VF_QUEUES + NFD_MAX_PF_QUEUES - 1) 

#define MAX_NUM_PCI_ISLS 4

#define _AUTO_MASK 1   // should come from config 


typedef struct prev_cnt_t {
    uint32_t rx_cnt[MAX_QUEUE_NUM+1];
    uint32_t tx_cnt[MAX_QUEUE_NUM+1];
} prev_cnt_t;

prev_cnt_t prev_cnt[MAX_NUM_PCI_ISLS];


typedef struct queue_pend_intr_t {
    uint64_t rx;
    uint64_t tx;
} queue_pend_intr_t;

__shared __lmem queue_pend_intr_t queue_pending_intr[MAX_NUM_PCI_ISLS];


typedef struct queue_en_t {
    uint64_t rx;
    uint64_t tx;
} queue_en_t;

__shared __lmem queue_en_t queue_enabled[MAX_NUM_PCI_ISLS];


typedef struct vec_num_per_queue_t {
    unsigned int vec_num[MAX_QUEUE_NUM+1];
} vec_num_per_queue_t;

__shared __imem_n(0) vec_num_per_queue_t vector_num_per_q[MAX_NUM_PCI_ISLS];

// assert on size of rx_queue_pending_intr and rx_queue_enabled if NFD_OUT_MAX_QUEUES !=64



#define __NFD_OUT_ATOMICS_SZ 16   /// why defined as 8 ?
__intrinsic uint32_t
__get_rx_queue_cnt(unsigned int pcie_nr, unsigned int queue)
{
    unsigned int addr_hi;
    unsigned int addr_lo;
    __xread uint32_t rdata;
    SIGNAL rsig;

    addr_hi = (0x84 | pcie_nr) << 24;
    addr_lo = (queue * __NFD_OUT_ATOMICS_SZ) + NFD_OUT_ATOMICS_SENT;

    __asm mem[atomic_read, rdata, addr_hi, <<8, addr_lo, 1], ctx_swap[rsig];

    return(rdata);
}

int 
queue_is_pf(unsigned int queue_num)
{
    if (queue_num >= (NFD_MAX_VF_QUEUES * NFD_MAX_VFS)) {
        return 1;
    } else {
        return 0;
    }
}

__intrinsic void 
msix_gen_set_rx_queue_enabled(unsigned int pcie_isl, unsigned int qnum, unsigned int en) 
{
    if (en==1) {
       queue_enabled[pcie_isl].rx |= (1 << qnum); 
    } else {
       queue_enabled[pcie_isl].rx &= ~(1 << qnum); 
    }
    
}

__intrinsic void 
msix_gen_set_rx_queue_vector(unsigned int pcie_isl, unsigned int qnum, unsigned int vector)
{
    vector_num_per_q[pcie_isl].vec_num[qnum]=vector;
}


struct rx_ring_vector_t {
    uint8_t data[64];
} rx_ring_vector_t;

__intrinsic void
rx_queue_monitor_update_config(unsigned int pcie_isl, 
                               unsigned int vnic, 
                               __xread unsigned int cfg_bar_data[6], 
                               __xread unsigned int rx_ring_vector_data[16])
{
    unsigned int indx,num_of_queues;
    unsigned int queue_num;
    unsigned int temp;
    unsigned int vector_num;
    __align4 unsigned int word_data[2];
    __align4 unsigned long long int bit_data;
    
    __lmem struct rx_ring_vector_t rx_ring_vector_data_byte;

    word_data[0]=cfg_bar_data[5];
    word_data[1]=cfg_bar_data[4];
    bit_data=*((unsigned long long int*)&word_data[0]);

    reg_cp((void *)&rx_ring_vector_data_byte, (void *)&rx_ring_vector_data,64);

    if (vnic==NFD_MAX_VFS) {
        // this is a PF
        num_of_queues = NFD_MAX_PF_QUEUES;
    } else {
        // this is a VF
        num_of_queues = NFD_MAX_VF_QUEUES;
    }

    // For every possible queue for this function, update enabled RX queues and vectors
    for (indx=0; indx<num_of_queues; indx++) {
        if (indx==32) {
           bit_data = bit_data >> 32;
        }
        
        queue_num = indx+vnic*NFD_MAX_VF_QUEUES;

        if ((bit_data & (1<<( indx&~(0x20000) ))) > 0) {
            // enable queue
            msix_gen_set_rx_queue_enabled(pcie_isl, queue_num, 1);
            // flip byte order to get the correct vector
            temp=(indx & ~3) + (3-(indx & 3));
            vector_num = rx_ring_vector_data_byte.data[temp];
            msix_gen_set_rx_queue_vector(pcie_isl, queue_num, vector_num);
        } else {
            // disable queue
            msix_gen_set_rx_queue_enabled(pcie_isl, queue_num, 0);
        }
    }
}

void 
rx_queue_monitor_init(unsigned int pcie_isl) 
{
    int qnum;

    queue_pending_intr[pcie_isl].rx = 0;
    queue_enabled[pcie_isl].rx = 0;

    for (qnum=0 ; qnum <= MAX_QUEUE_NUM ; qnum++) {
        prev_cnt[pcie_isl].rx_cnt[qnum] = 0;
    }

    local_csr_write(NFP_MECSR_MAILBOX_3, 0);
    local_csr_write(NFP_MECSR_MAILBOX_2, 0xbaba);

}

void 
rx_queue_monitor(unsigned int pcie_isl) 
{
    int tmp;
    int qnum, vf_num;

    uint32_t rx_cnt;

    for (qnum=0 ; qnum <= MAX_QUEUE_NUM ; qnum++) {
        if ((queue_enabled[pcie_isl].rx & (1<<qnum)) != 0) {
            rx_cnt=__get_rx_queue_cnt(pcie_isl+4,qnum);
            if (rx_cnt != prev_cnt[pcie_isl].rx_cnt[qnum]) {
                queue_pending_intr[pcie_isl].rx |= (1 << qnum);
                prev_cnt[pcie_isl].rx_cnt[qnum] = rx_cnt;
            }
          
            if ((queue_pending_intr[pcie_isl].rx & (1<<qnum)) != 0) {
                // attempt to send an interrupt
                if (queue_is_pf(qnum)==1) {
                    tmp = msix_pf_send(pcie_isl+4, vector_num_per_q[pcie_isl].vec_num[qnum], _AUTO_MASK);
                } else {
                    vf_num = qnum / NFD_MAX_VF_QUEUES;
                    tmp = msix_vf_send(pcie_isl+4, vf_num, vector_num_per_q[pcie_isl].vec_num[qnum], _AUTO_MASK);
                }
      
                // clear the pending bit if send was successful
                if (tmp==0) {
                    queue_pending_intr[pcie_isl].rx &= ~(1 << qnum);
                }
            }
        }
    }
}
