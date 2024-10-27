#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/portmacro.h"
#include "driver/uart.h"
#include <string.h>
#include <stdio.h>
#include "driver/gpio.h"

#include "uartneo6m.h"

QueueHandle_t uart_msg_queue;

esp_err_t module_gps_uart_int()
{

  gpio_reset_pin(UART_RST);
  /* Set the GPIO as a push/pull output */
  gpio_set_direction(UART_RST, UART_RST);
  gpio_set_level(UART_RST, 0);
  vTaskDelay(pdMS_TO_TICKS(200));
  gpio_set_level(UART_RST, 1);
  vTaskDelay(pdMS_TO_TICKS(1000));

  esp_err_t ret;
  uart_config_t uart_config_gps = {
      .baud_rate = 9600,
      .data_bits = UART_DATA_8_BITS,
      .parity = UART_PARITY_DISABLE,
      .stop_bits = UART_STOP_BITS_1,
      .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
      .source_clk = UART_SCLK_DEFAULT,
  };

  // Install UART driver, and get the queue.
  ret = uart_driver_install(UART_NUM_1, 1024, 1024, 30, &uart_msg_queue, 0);
  if (ret != ESP_OK)
  {
    printf("Failed to install UART driver %s\n", esp_err_to_name(ret));
    return ret;
  }
  ret = uart_param_config(UART_NUM_1, &uart_config_gps);
  if (ret != ESP_OK)
  {
    printf("Failed to configure %s\n", esp_err_to_name(ret));
    return ret;
  }
  // Set UART pins (using UART0 default pins ie no changes.)
  ret = uart_set_pin(UART_NUM_1, UART_TX, UART_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
  if (ret != ESP_OK)
  {
    printf("Failed to set pin %s\n", esp_err_to_name(ret));
    return ret;
  }
  return ret;
}

esp_err_t AT_tx(char *cmd, uint64_t delay_ms)
{
  esp_err_t ret;
  ret = uart_write_bytes(UART_NUM_1, cmd, strlen(cmd));
  vTaskDelay(pdMS_TO_TICKS(delay_ms));
  return ret;
}

int AT_rx(char *buf, size_t buf_size)
{
  uart_flush_input(UART_NUM_1);
  return uart_read_bytes(UART_NUM_1, buf, 512, 1000);
}

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/