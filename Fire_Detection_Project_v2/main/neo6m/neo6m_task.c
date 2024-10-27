#include "neo6m.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <string.h>
#include "esp_log.h"
#include <math.h>
#include "wifi_mqtt/mqtt.h"
#include "neo6m/uartneo6m.h"
#include "relay/relay.h"
#include "sensor_data.h"
extern bool is_wifi_connect;
gps_coordinates_t coordinates;
TaskHandle_t neo6m_TaskHandle = NULL;
TaskHandle_t neo6m_pulish_data_TaskHandle = NULL;
static const char *TAG = "neo6m_task";
gps_coordinates_t neo6m_values;
extern QueueHandle_t neo6m_queue;
// Tạo một task để đọc GPS liên tục
void neo6m_task(void *pvParameter)
{
  gps_coordinates_t neo6m_data;
  esp_err_t ret;
  for (;;)
  {
    ret = neo6m_get_coordinates(&neo6m_values);
    ESP_LOGI(TAG, "Latitude: %.5f, Longitude: %.5f\n", neo6m_values.latitude, neo6m_values.longitude);
    neo6m_data.latitude = neo6m_values.latitude;
    neo6m_data.longitude = neo6m_values.longitude;
    xQueueSend(neo6m_queue, &neo6m_data, portMAX_DELAY);
    vTaskDelay(5000 / portTICK_PERIOD_MS);
  }
}

// Hàm khởi chạy task GPS
int neo6m_task_init(void)
{
  BaseType_t taskCreated = xTaskCreate(neo6m_task, "neo6m_task", 4096, NULL, 10, &neo6m_TaskHandle);
  if (taskCreated != pdTRUE)
  {
    ESP_LOGE(TAG, "Failed to create task");
    return taskCreated;
  }
  return taskCreated;
}
