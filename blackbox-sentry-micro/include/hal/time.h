// include/hal/time.h
#ifndef HAL_TIME_H
#define HAL_TIME_H

#include <stdint.h>

// Get monotonic time in milliseconds
uint64_t hal_get_time_ms(void);

// Blocking sleep
void hal_sleep_ms(uint32_t ms);

#endif