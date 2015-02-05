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

/**
 *Read an atomic counter stored in local CTM
 * @param base      Start address of structure to increment
 * @param queue     Queue within structure to increment
 * @param val       Value to add
 * @param counter   Counter to increment
 *
 * XXX replace this command with suitable flowenv alternative when available.
 */
__intrinsic unsigned int
_read_imm(unsigned int base, unsigned int queue, unsigned int counter)
{
    unsigned int ind;
    __xread uint32_t rdata; 
    SIGNAL rsig;

    ctassert(__is_ct_const(counter));

    queue = queue * NFD_OUT_ATOMICS_SZ | counter;
    ind = (NFP_MECSR_PREV_ALU_LENGTH(8) | NFP_MECSR_PREV_ALU_OV_LEN |
           NFP_MECSR_PREV_ALU_OVE_DATA(2));

   // __asm alu[--, ind, or, val, <<16];
    //__asm mem[atomic_read, rdata, base, queue, 1], indirect_ref,;
    //__asm mem[atomic_read, rdata, base, queue, 4], ctx_swap[rsig]
   // mem_read_atomic(&rdata, (__mem *void)(base + queue), 4) ;  
    mem_read_atomic(&rdata, (base + queue), 4) ;  

    return rdata;
}

      unsigned int qnum, vf_num;
__gpr uint32_t     rx_cnt;
      uint32_t     prev_rx_cnt[MAX_QUEUE_NUM+1];
__gpr uint64_t     pending_msi;
__gpr uint64_t     rx_queue_enabled;
      unsigned int vector_num_per_q[MAX_QUEUE_NUM+1];

__emem uint32_t    msi_debug[100];

// assert on size of pending_msi if NFD_OUT_MAX_QUEUES !=64

uint32_t 
read_rx_cnt(unsigned int qnum) {
    return (_read_imm(NFD_OUT_CREDITS_BASE, qnum, NFD_OUT_ATOMICS_SENT));
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

    if (queue_is_pf(qnum)==1) {
        mask = msix_status(_PCIE_NR, vector_num_per_q[qnum]);
    } else {
        vf_num = qnum / NFD_MAX_VF_QUEUES;
        mask = msi_vf_status(_PCIE_NR, vf_num, vector_num_per_q[qnum]);
    }

    if (mask == 1) {
        return 1;
    } else {
        return 0;
    }
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
int_gen_set_rx_queue_vetcor(int qnum, int vector)
{
    vector_num_per_q[qnum]=vector;
}


void 
msix_gen_init() 
{
    pending_msi = 0;
    msi_debug[0] = 0x01020304;
}

void 
msix_gen_loop() 
{
    for (qnum=0 ; qnum <= MAX_QUEUE_NUM ; qnum++) {
        if ((rx_queue_enabled & (1<<qnum)) != 0) {
            rx_cnt=read_rx_cnt(qnum);
            if (rx_cnt != prev_rx_cnt[qnum]) {
                pending_msi = pending_msi | (1 << qnum);
                prev_rx_cnt[qnum] = rx_cnt;
            }
          
            if ((pending_msi & (1<<qnum)) != 0) {
                if (queue_vector_is_masked(qnum)==0) {
                    if (queue_is_pf(qnum)==1) {
                        msix_send(_PCIE_NR, vector_num_per_q[qnum], _AUTO_MASK);
                    } else {
                        vf_num = qnum / NFD_MAX_VF_QUEUES;
                        msi_vf_send(_PCIE_NR, vector_num_per_q[qnum], vf_num, _AUTO_MASK);
                        pending_msi = pending_msi & ~(1 << qnum);
                    }
                }
            }
        }
    }
}
