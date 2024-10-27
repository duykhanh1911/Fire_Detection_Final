/***************************************************************************
 * @file 	mqtt.c
 * @author 	tuha
 * @date 	5 July 2023
 * @brief	module for send data through mqtt
 *       	and receive command from host main through mqtt
 ***************************************************************************/

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/

#include "mqtt_client.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "cJSON.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "esp_system.h"
#include "esp_mac.h"
#include "sensor_data.h"
#include "wifi_mqtt/mqtt.h"
#include "esp_err.h"
#include "bme680/bme680.h"
#include "neo6m/neo6m.h"
/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/

static uint8_t trans_code = 0;
static esp_mqtt_client_handle_t client;

static char cMac_str[13];
static char cTopic_pub[64] = "FIRE_DETECTION/DeviceID/Telemetry";
static char cTopic_sub[64] = "FIRE_DETECTION/DeviceID/Command";
static char rxBuffer_MQTT[800];
static QueueHandle_t mqtt_cmd_queue;

static const char *TAG_MQTT = "MQTT";
static const char *TAG = "public_data_task";
extern QueueHandle_t bme680_queue;
extern QueueHandle_t neo6m_queue;
/****************************************************************************/
/***        Event Handler                                                 ***/
/****************************************************************************/

void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
  esp_mqtt_event_handle_t event = event_data;
  esp_mqtt_client_handle_t client = event->client;

  switch ((esp_mqtt_event_id_t)event_id)
  {
  case MQTT_EVENT_CONNECTED:
    ESP_LOGI(TAG_MQTT, "MQTT_EVENT_CONNECTED");

    snprintf(cTopic_sub, sizeof(cTopic_sub), "FIRE_DETECTION/%s/Command", cMac_str);
    esp_mqtt_client_subscribe(client, cTopic_sub, 0);
    ESP_LOGI(TAG_MQTT, "Topic subscribe: %s\n", cTopic_sub);

    break;

  case MQTT_EVENT_DISCONNECTED:
    ESP_LOGI(TAG_MQTT, "MQTT_EVENT_DISCONNECTED");
    wifi_mqtt_reconnect();
    break;

  case MQTT_EVENT_SUBSCRIBED:
    ESP_LOGI(TAG_MQTT, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
    break;

  case MQTT_EVENT_UNSUBSCRIBED:
    ESP_LOGI(TAG_MQTT, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
    break;

  case MQTT_EVENT_PUBLISHED:
    ESP_LOGI(TAG_MQTT, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
    break;

  case MQTT_EVENT_DATA:
    if ((event->data) != NULL)
    {
      ESP_LOGI(TAG_MQTT, "MQTT_EVENT_DATA");
      snprintf(rxBuffer_MQTT, event->data_len + 1, event->data);
      xQueueSend(mqtt_cmd_queue, &rxBuffer_MQTT, portMAX_DELAY);
    }

    break;

  case MQTT_EVENT_ERROR:
    ESP_LOGI(TAG_MQTT, "MQTT_EVENT_ERROR");
    break;

  default:
    ESP_LOGI(TAG_MQTT, "Other event id:%d", event->event_id);
    break;
  }
}

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

void wifi_mqtt_reconnect()
{
  /*Config mqtt client*/
  esp_mqtt_client_config_t mqtt_cfg = {
      .broker = {
          .address = {
              .hostname = "103.1.238.175",
              .transport = MQTT_TRANSPORT_OVER_TCP,
              .port = 1883,

          },
      },
      .credentials = {
          .username = "test",
          .client_id = "WICOMLAB",
          .set_null_client_id = false,
          .authentication = {
              .password = "testadmin",
          },
      },
      .network = {
          .disable_auto_reconnect = false,
          .timeout_ms = 10000,
      },
  };
  client = esp_mqtt_client_init(&mqtt_cfg);
  esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
  esp_mqtt_client_start(client);
}

void wifi_mqtt_func_init(void)
{
  esp_event_loop_create_default();
  /*Config mqtt client*/
  esp_mqtt_client_config_t mqtt_cfg = {
      .broker = {
          .address = {
              .hostname = "103.1.238.175",
              .transport = MQTT_TRANSPORT_OVER_TCP,
              .port = 1883,

          },
      },
      .credentials = {
          .username = "test",
          .client_id = "WICOMLAB",
          .set_null_client_id = false,
          .authentication = {
              .password = "testadmin",
          },
      },
      .network = {
          .disable_auto_reconnect = false,
          .timeout_ms = 10000,
      },
  };
  client = esp_mqtt_client_init(&mqtt_cfg);
  esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
  esp_mqtt_client_start(client);

  /* Get mac Address and set topic*/
  uint8_t u8mac[6];
  esp_efuse_mac_get_default(u8mac);
  snprintf(cMac_str, sizeof(cMac_str), "%02X%02X%02X%02X%02X%02X", u8mac[0], u8mac[1], u8mac[2], u8mac[3], u8mac[4], u8mac[5]);
  snprintf(cTopic_pub, sizeof(cTopic_pub), "FIRE_DETECTION/%s/Telemetry", cMac_str);

  ESP_LOGI(TAG_MQTT, "Topic publish: %s\n", cTopic_pub);

  mqtt_cmd_queue = xQueueCreate(2, sizeof(cJSON *));
}

void publish_data_task(void *parameter) // Chuan hoa lai MQTT Publish task - Vu
{
  bme680_values_float_t bme680_data;
  gps_coordinates_t neo6m_data;
  while (1)
  {
    if (uxQueueMessagesWaiting(bme680_queue) && uxQueueMessagesWaiting(neo6m_queue) != 0)
    {
      if (xQueueReceive(bme680_queue, &bme680_data, portMAX_DELAY) == pdPASS && xQueueReceive(neo6m_queue, &neo6m_data, portMAX_DELAY) == pdPASS)
      {
        esp_err_t error = ESP_OK;
        ESP_LOGI(TAG, " %.2f °C, %.2f %%, %.2f hPa, %.2f KOhm\n,%.2f ,%.2f \n", bme680_data.temperature, bme680_data.humidity, bme680_data.pressure, bme680_data.gas_resistance, neo6m_data.latitude, neo6m_data.longitude);
        cJSON *json_data = cJSON_CreateObject(); // Create a JSON object for the data
        cJSON_AddStringToObject(json_data, "Device_ID", cMac_str);
        cJSON_AddStringToObject(json_data, "Connect_type", "WIFI");
        cJSON *values = cJSON_AddObjectToObject(json_data, "Values"); // Create a nested JSON object for the 'values' field
        cJSON_AddNumberToObject(values, "Temp", bme680_data.temperature);
        cJSON_AddNumberToObject(values, "Humi", bme680_data.humidity);
        cJSON_AddNumberToObject(values, "Press", bme680_data.pressure);
        cJSON_AddNumberToObject(values, "VOC", bme680_data.gas_resistance);
        cJSON_AddNumberToObject(values, "Latitude", neo6m_data.latitude);
        cJSON_AddNumberToObject(values, "Longitude", neo6m_data.longitude);
        cJSON_AddNumberToObject(json_data, "trans_code", trans_code++);

        char *json_str = cJSON_PrintUnformatted(json_data);                         // Convert the JSON object to a string
        error = esp_mqtt_client_publish(client, cTopic_pub, json_str, 0, QoS_1, 0); // Publish the JSON string via MQTT
        cJSON_Delete(json_data);
        free(json_str);

        if (error == ESP_FAIL)
        {
          ESP_LOGE(__func__, "MQTT client publish message unsuccessfully...");
        }
        else
        {
          ESP_LOGI(__func__, "MQTT client publish message successfully.");
        }
        vTaskDelay((TickType_t)(1000 / portTICK_PERIOD_MS));
      }
    }
    else
    {
      vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
  }
}

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/