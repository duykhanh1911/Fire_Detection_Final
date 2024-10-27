#ifndef __UARTNEO6M_H__
#define __UARTNEO6M_H__

#include "io_def/io_def.h"

#define BUF_SIZE 1024

esp_err_t module_gps_uart_int();

esp_err_t AT_tx(char *cmd, uint64_t delay_ms);

int AT_rx(char *buf, size_t buf_size);

#endif //__H__