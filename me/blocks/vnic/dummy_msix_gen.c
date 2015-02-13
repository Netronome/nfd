/* You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @file   dummy_msix_gen.c       
 * @brief  MSI/MSI-X test ME
 */

#include <nfp.h>
#include <stdint.h>
#include <types.h>

#include <nfp/me.h>

#include <nfp6000/nfp_me.h>
#include "msix.h"

void main(void)
{

  __gpr unsigned int reg_a;
  __gpr unsigned int tmp;
  __gpr unsigned int pcie_nr;
  __gpr unsigned int vf_fn_nr;
  __gpr unsigned int int_nr;
  __gpr unsigned int mask;
  __gpr unsigned int state;

  if (ctx() == 0) {
    
    state = 0 ;
    local_csr_write(NFP_MECSR_MAILBOX_0, 0x01020316); 
    local_csr_write(NFP_MECSR_MAILBOX_1, 0); 
    local_csr_write(NFP_MECSR_MAILBOX_2, 0); 
    local_csr_write(NFP_MECSR_MAILBOX_3, 0); 

    for(;;) {
      
      reg_a = local_csr_read(NFP_MECSR_MAILBOX_1);   
      
      if (state == 0) {
         if ((reg_a & 0x80000000) > 0) {
            state = 1;
            pcie_nr=(reg_a & 0xFF0000) >> 16;
            int_nr=(reg_a & 0xFF00) >> 8;
            mask=(reg_a & 0xFF) >> 0;
         
            if (((reg_a & 0xF000000) >> 24) == 0) {
               msix_send(pcie_nr, int_nr, mask);
               local_csr_write(NFP_MECSR_MAILBOX_2, (local_csr_read(NFP_MECSR_MAILBOX_2) +1) );
            }

            if (((reg_a & 0xF000000) >> 24) == 2) {
               local_csr_write(NFP_MECSR_MAILBOX_3, msix_status(pcie_nr, int_nr));
               local_csr_write(NFP_MECSR_MAILBOX_2, (local_csr_read(NFP_MECSR_MAILBOX_2) + 0x10000) );
            }
  
            if (((reg_a & 0xF000000) >> 24) == 4) {
               vf_fn_nr = pcie_nr;
               pcie_nr = 4;
               msi_vf_send(pcie_nr, vf_fn_nr, int_nr, mask);
               local_csr_write(NFP_MECSR_MAILBOX_2, (local_csr_read(NFP_MECSR_MAILBOX_2) +1) );
            }

            if (((reg_a & 0xF000000) >> 24) == 6) {
               vf_fn_nr = pcie_nr;
               pcie_nr = 4;
               local_csr_write(NFP_MECSR_MAILBOX_3, msi_vf_status(pcie_nr, vf_fn_nr, int_nr));
               local_csr_write(NFP_MECSR_MAILBOX_2, (local_csr_read(NFP_MECSR_MAILBOX_2) + 0x10000) );
            }
    
         }
      } else {
        if ((reg_a & 0x80000000) == 0) {
           state = 0; 
        }
      }
    }
  }


  for(;;) 
  {
     ctx_wait(kill);
  }

}
