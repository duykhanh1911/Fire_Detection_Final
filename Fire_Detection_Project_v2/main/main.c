#include <stdio.h>
#include <esp_log.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "neo6m/uartneo6m.h"
#include "bme680/bme680.h"
#include "wifi_mqtt/button.h"
#include "wifi_mqtt/mqtt.h"
#include "wifi_mqtt/wifi.h"
#include "wifi_mqtt/nvs.h"
#include "io_def/io_def.h"
#include "relay/relay.h"
#include "neo6m/neo6m.h"
#include "sensor_data.h"
extern int bme680_task_init();
extern int neo6m_task_init();
extern bool is_wifi_connect;
QueueHandle_t bme680_queue;
QueueHandle_t neo6m_queue;
TaskHandle_t pub_data_TaskHandle = NULL;
void app_main()
{
  relay_init();
  button_init(CONFIG_WIFI_BUTTON);
  nvs_flash_func_init();
  wifi_func_init();

  if (is_wifi_connect)
    wifi_mqtt_func_init();

  vTaskDelay(pdMS_TO_TICKS(2000));
  module_gps_uart_int();
  bme680_queue = xQueueCreate(10, sizeof(bme680_values_float_t));
  neo6m_queue = xQueueCreate(10, sizeof(gps_coordinates_t));
  bme680_task_init();
  neo6m_task_init();
  xTaskCreate(publish_data_task, "publish_data_task", 64 * 1024, NULL, 5, &pub_data_TaskHandle);
}
