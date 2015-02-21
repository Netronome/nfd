/**
  * update rx queue monitor from configuration BAR read info
  */
__intrinsic void
rx_queue_monitor_update_config(unsigned int vnic, __xread unsigned int cfg_bar_data[6], __xread unsigned int rx_ring_vector_data[16]);

/**
  * init routine to set initial internal state
  */
void rx_queue_monitor_init();

/**
  * main loop to get state of rx queue and generate interrupts
  */
void rx_queue_monitor();

