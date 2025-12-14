// include/hal/sys.h
#ifndef HAL_SYS_H
#define HAL_SYS_H

#include <stddef.h>

/**
 * @brief Fills the buffer with a unique device identifier.
 * On Linux, this is the Hostname or MAC address.
 */
void hal_sys_get_device_id(char* buffer, size_t max_len);

#endif