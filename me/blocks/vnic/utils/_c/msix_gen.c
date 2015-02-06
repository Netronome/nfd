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

#define MAX_QUEUE_NUM (NFD_MAX_VFS*NFD_MAX_VF_QUEUES + NFD_MAX_PF_QUEUES - 1) 

#define _PCIE_NR   4   // should be defined externally
#define _AUTO_MASK 1   // defined externally ? 

      unsigned int qnum, vf_num;
__gpr uint32_t     rx_cnt;
      uint32_t     prev_rx_cnt[MAX_QUEUE_NUM+1];
__gpr uint64_t     pending_msi;
__gpr uint64_t     rx_queue_enabled;
      unsigned int vector_num_per_q[MAX_QUEUE_NUM+1];
// assert on size of pending_msi and rx_queue_enabled if NFD_OUT_MAX_QUEUES !=64

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


int 
queue_is_pf(unsigned int queue_num)
{
    if (queue_num >= (NFD_MAX_VF_QUEUES * NFD_MAX_VFS)) {
        return 1;
    } else {
        return 0;
    }
}

int 
queue_vector_is_masked(unsigned int qnum) 
{
    int mask;
    int _vf_num;

    if (queue_is_pf(qnum)==1) {
        mask = msix_status(_PCIE_NR, vector_num_per_q[qnum]);
    } else {
        _vf_num = qnum / NFD_MAX_VF_QUEUES;
        mask = msi_vf_status(_PCIE_NR, _vf_num, vector_num_per_q[qnum]);
    }

    return (mask);
}

void 
set_rx_queue_enabled(int qnum, int en) 
{
    if (en==1) {
       rx_queue_enabled = rx_queue_enabled | (1 << qnum); 
    } else {
       rx_queue_enabled = rx_queue_enabled & !(1 << qnum); 
    }
}

void 
set_rx_queue_vector(int qnum, int vector)
{
    vector_num_per_q[qnum]=vector;
}


void 
msix_gen_init() 
{
    pending_msi = 0;
    rx_queue_enabled = 0;

    for (qnum=0 ; qnum <= MAX_QUEUE_NUM ; qnum++) {
        prev_rx_cnt[qnum] = 0;
    }

    //set_rx_queue_enabled(0,1);    
    //set_rx_queue_enabled(1,1);   
    //set_rx_queue_vector(0,2);
    //set_rx_queue_vector(1,3);

}

void 
msix_gen_loop() 
{
    for (qnum=0 ; qnum <= MAX_QUEUE_NUM ; qnum++) {
        if ((rx_queue_enabled & (1<<qnum)) != 0) {
            rx_cnt=__get_rx_queue_cnt(_PCIE_NR,qnum);
            if (rx_cnt != prev_rx_cnt[qnum]) {
                pending_msi = pending_msi | (1 << qnum);
                prev_rx_cnt[qnum] = rx_cnt;
            }
          
            if ((pending_msi & (1<<qnum)) != 0) {
                if (queue_vector_is_masked(qnum)==0) {
                    //local_csr_write(NFP_MECSR_MAILBOX_2, local_csr_read(NFP_MECSR_MAILBOX_2)+1);
                    if (queue_is_pf(qnum)==1) {
                        msix_send(_PCIE_NR, vector_num_per_q[qnum], _AUTO_MASK);
                    } else {
                        vf_num = qnum / NFD_MAX_VF_QUEUES;
                        msi_vf_send(_PCIE_NR, vector_num_per_q[qnum], vf_num, _AUTO_MASK);
                    }
      
                    pending_msi = pending_msi & ~(1 << qnum);
                    
                }
            }
        }
    }
}
