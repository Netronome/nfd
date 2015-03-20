/**
  * init routine to set initial internal state
  */
void msix_qmon_init(unsigned int pcie_isl);

/**
  * Call on reconfiguration.
  * 
  * @cfg_bar       Pointer to control Bar for this vNIC
  * @cfg_bar_data  First 6 words of the control BAR
  */
__intrinsic void msix_reconfig(unsigned int pcie_isl, unsigned int vnic,
                   __mem char *cfg_bar, __xread unsigned int cfg_bar_data[6]);


/**
  * main loop to get state of rx queue and generate interrupts
  */
void msix_qmon_loop(unsigned int pcie_isl);

