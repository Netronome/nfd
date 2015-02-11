/**
  * update misx config from configuration BAR read info
  */
__intrinsic void
msix_gen_update_config(unsigned int vnic, __xread unsigned int cfg_bar_data[6], __xread unsigned int rx_ring_vector_data[16]);

/**
  * init routine to set initial internal state
  */
void msix_gen_init();

/**
  * main loop to get state of rx queue and generate interrupts
  */
void msix_gen_loop();

