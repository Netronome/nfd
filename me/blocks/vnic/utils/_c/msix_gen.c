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
  need to clear all pending interrupts when function comes up or down (what if the state didn't change? save state?)
*/

#define MAX_QUEUE_NUM (NFD_MAX_VFS*NFD_MAX_VF_QUEUES + NFD_MAX_PF_QUEUES - 1) 

#define _PCIE_NR   4   // should be defined externally
#define _AUTO_MASK 1   // defined externally ? 
//#define _AUTO_MASK 0   // defined externally ? 

      unsigned int qnum, vf_num;
__gpr uint32_t     rx_cnt;
      uint32_t     prev_rx_cnt[MAX_QUEUE_NUM+1];
__shared __gpr uint64_t     rx_queue_pending_intr;

__shared __gpr       uint64_t     rx_queue_enabled;
__shared __imem_n(0) unsigned int vector_num_per_q[MAX_QUEUE_NUM+1];
//__shared lmem unsigned int vector_num_per_q[MAX_QUEUE_NUM+1];
// assert on size of rx_queue_pending_intr and rx_queue_enabled if NFD_OUT_MAX_QUEUES !=64

// debug
//#define MSIX_GEN_DEBUG_EN

#ifdef MSIX_GEN_DEBUG_EN
//__shared __lmem __align4 uint32_t msix_debug[100];  
__shared __lmem uint32_t msix_debug[100];  
__xwrite uint64_t debug_data[8];
__xwrite uint32_t debug_data_32[8];
int i;
#endif

#define __NFD_OUT_ATOMICS_SZ 16   /// why defined as 8 ?
__intrinsic uint32_t
__get_rx_queue_cnt(unsigned int pcie_isl, unsigned int queue)
{
    unsigned int addr_hi;
    unsigned int addr_lo;
    __xread uint32_t rdata;
    SIGNAL rsig;

    addr_hi = (0x84 | pcie_isl) << 24;
    addr_lo = (queue * __NFD_OUT_ATOMICS_SZ) + NFD_OUT_ATOMICS_SENT;

    __asm mem[atomic_read, rdata, addr_hi, <<8, addr_lo, 1], ctx_swap[rsig];

    return(rdata);
}

#ifdef MSIX_GEN_DEBUG_EN
__intrinsic void
update_debug() 
{
    int word;
    int byte_num;
    uint32_t temp;

    for (i=0; i<8; i++) {
        debug_data[i]=0;
    }

    for (i=0; i<8; i++) {
        debug_data_32[i]=0;
    }

    debug_data[0] = rx_queue_pending_intr;
    debug_data[1] = rx_queue_enabled;
    mem_write64((void*)&debug_data, (__mem void*)&msix_debug[4], sizeof debug_data);

    byte_num=0;
    word=0;
    temp=0;
    for (i=0; i<=MAX_QUEUE_NUM; i++) {
        temp=temp + (vector_num_per_q[i] << (byte_num*8));
        debug_data_32[word]=temp;
        byte_num++;
        if (byte_num==4) {
            byte_num=0;
            temp=0;
            word++;
        }
    }
    mem_write32((void*)&debug_data_32, (__mem void*)&msix_debug[8], sizeof debug_data_32);
}
#endif

int 
queue_is_pf(unsigned int queue_num)
{
    if (queue_num >= (NFD_MAX_VF_QUEUES * NFD_MAX_VFS)) {
        return 1;
    } else {
        return 0;
    }
}

/*int 
queue_vector_is_masked(unsigned int qnum) 
{
    int mask;
    int _vf_num;

    int tmp;

    if (queue_is_pf(qnum)==1) {
        mask = msix_status(_PCIE_NR, vector_num_per_q[qnum]);
    } else {
        if (qnum==0) {
            _vf_num = 0;
        } else {
            _vf_num = qnum / NFD_MAX_VF_QUEUES;
        }
        mask = msi_vf_status(_PCIE_NR, _vf_num, vector_num_per_q[qnum]);
    }

    tmp=local_csr_read(NFP_MECSR_MAILBOX_3);
    tmp = tmp & ~(1<<qnum);
    tmp = tmp | (mask<<qnum);
    local_csr_write(NFP_MECSR_MAILBOX_3,tmp);
    return (mask);
}
*/

__intrinsic void 
msix_gen_set_rx_queue_enabled(int qnum, int en) 
{
    if (en==1) {
       rx_queue_enabled = rx_queue_enabled | (1 << qnum); 
    } else {
       rx_queue_enabled = rx_queue_enabled & ~(1 << qnum); 
    }
    
//    local_csr_write(NFP_MECSR_MAILBOX_2, rx_queue_enabled);
#ifdef MSIX_GEN_DEBUG_EN
    update_debug();
#endif

}

__intrinsic void 
msix_gen_set_rx_queue_vector(int qnum, int vector)
{
    vector_num_per_q[qnum]=vector;
#ifdef MSIX_GEN_DEBUG_EN
   update_debug();
#endif

}


struct rx_ring_vector_t {
    uint8_t data[64];
} rx_ring_vector_t;

__intrinsic void
rx_queue_monitor_update_config(unsigned int vnic, __xread unsigned int cfg_bar_data[6], __xread unsigned int rx_ring_vector_data[16])
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

//    local_csr_write(NFP_MECSR_MAILBOX_2, cfg_bar_data[4]);

    if (vnic==NFD_MAX_VFS) {
        // this is a PF
        num_of_queues = NFD_MAX_PF_QUEUES;
    } else {
        // this is a VF
        num_of_queues = NFD_MAX_VF_QUEUES;
    }


    // For every possible queue fo this function, update enabled RX queues and vectors
    for (indx=0; indx<num_of_queues; indx++) {
        if (indx==32) {
           bit_data = bit_data >> 32;
        }
        
        queue_num = indx+vnic*NFD_MAX_VF_QUEUES;

        if ((bit_data & (1<<( indx&~(0x20000) ))) > 0) {
            // enable queue
            msix_gen_set_rx_queue_enabled(queue_num,1);
            // flip byte order to get the correct vector
            temp=(indx & ~3) + (3-(indx & 3));
            vector_num = rx_ring_vector_data_byte.data[temp];
            msix_gen_set_rx_queue_vector(queue_num,vector_num);
        } else {
            // disable queue
            msix_gen_set_rx_queue_enabled(queue_num,0);
        }
    }
    
    //local_csr_write(NFP_MECSR_MAILBOX_2, rx_queue_enabled);
}

void 
rx_queue_monitor_init() 
{
    rx_queue_pending_intr = 0;
    rx_queue_enabled = 0;

    for (qnum=0 ; qnum <= MAX_QUEUE_NUM ; qnum++) {
        prev_rx_cnt[qnum] = 0;
    }

    local_csr_write(NFP_MECSR_MAILBOX_3, 0);
    local_csr_write(NFP_MECSR_MAILBOX_2, 0xbaba);

#ifdef MSIX_GEN_DEBUG_EN
    for (i=0; i<8; i++) {
        debug_data[i]=0;
    } 
    debug_data[0] = 0x1234acbd;
    debug_data[1] = MAX_QUEUE_NUM;
    mem_write64((void*)&debug_data, (__mem void*)&msix_debug, sizeof debug_data);
#endif
    
}

void 
rx_queue_monitor() 
{
    int tmp;

    for (qnum=0 ; qnum <= MAX_QUEUE_NUM ; qnum++) {
        if ((rx_queue_enabled & (1<<qnum)) != 0) {
            rx_cnt=__get_rx_queue_cnt(_PCIE_NR,qnum);
            if (rx_cnt != prev_rx_cnt[qnum]) {
                rx_queue_pending_intr |= (1 << qnum);
                prev_rx_cnt[qnum] = rx_cnt;
            }
          
            if ((rx_queue_pending_intr & (1<<qnum)) != 0) {
#if 0
                if (queue_vector_is_masked(qnum)==0) {
                    if (queue_is_pf(qnum)==1) {
                        msix_send(_PCIE_NR, vector_num_per_q[qnum], _AUTO_MASK);
                    } else {
                        vf_num = qnum / NFD_MAX_VF_QUEUES;
                        msi_vf_send(_PCIE_NR, vf_num, vector_num_per_q[qnum], _AUTO_MASK);
                    }
      
                    rx_queue_pending_intr &= ~(1 << qnum);
                    
                }
#endif
                // attempt to send an interrupt
                if (queue_is_pf(qnum)==1) {
                    tmp = msix_pf_send(_PCIE_NR, vector_num_per_q[qnum], _AUTO_MASK);
                } else {
                    vf_num = qnum / NFD_MAX_VF_QUEUES;
                    tmp = msix_vf_send(_PCIE_NR, vf_num, vector_num_per_q[qnum], _AUTO_MASK);
                }
      
                // clear the pending bit if send was successful
                if (tmp==0) {
                    rx_queue_pending_intr &= ~(1 << qnum);
                }
            }
        }
    }
}
