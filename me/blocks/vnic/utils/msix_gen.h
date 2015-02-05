/**
  * update internal state - called upon config change
  * set an internal state to indicate whether a queue is enabled or not  
  * @param qnum - queue number
  * @param en   - 1 queue is enabled, 0 queue is disabled
  */
void msix_gen_set_rx_queue_enabled(int qnum, int en);

/**
  * update internal state - called upon config change
  * set interrupt vector for a queue
  * @param qnum   - queue number
  * @param vector - interrupt vector to use for a queue 
  */
void msix_gen_set_rx_queue_vetcor(int qnum, int vector);

/**
  * init routine to set initial internal state
  */
void msix_gen_init();

/**
  * main loop to get state of rx queue and generate interrupts
  */
void msix_gen_loop();

