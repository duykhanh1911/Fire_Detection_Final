#include <stdio.h>
#include <esp_log.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "module_sim/A7680C.h"
#include "module_sim/uartdev.h"
#include "bme680/bme680.h"
#include "wifi_mqtt/button.h"
#include "wifi_mqtt/mqtt.h"
#include "wifi_mqtt/wifi.h"
#include "wifi_mqtt/nvs.h"
#include "io_def/io_def.h"
#include "relay/relay.h"

extern int bme680_task_init();
extern bool is_wifi_connect;
void app_main()
{
  relay_init();

  button_init(CONFIG_WIFI_BUTTON);
  nvs_flash_func_init();
  wifi_func_init();

  if (is_wifi_connect)
    wifi_mqtt_func_init();

  vTaskDelay(pdMS_TO_TICKS(2000));

  module_sim_uart_init();
  module_sim_connect_mqtt_broker();

  bme680_task_init();
}
