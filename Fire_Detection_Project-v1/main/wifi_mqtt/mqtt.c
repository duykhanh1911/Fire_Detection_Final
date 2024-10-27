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
#include "cJSON.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "esp_system.h"
#include "esp_mac.h"

#include "wifi_mqtt/mqtt.h"

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
      .broker.address.uri = BROKER_ADDRESS_URI,
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
      .broker.address.uri = BROKER_ADDRESS_URI,
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

void wifi_pub_data(float temp, float humi, float press, float voc)
{
  cJSON *json_data = cJSON_CreateObject(); // Create a JSON object for the data
  cJSON_AddStringToObject(json_data, "Device_ID", cMac_str);
  cJSON_AddStringToObject(json_data, "Connect_type", "WIFI");
  cJSON *values = cJSON_AddObjectToObject(json_data, "Values"); // Create a nested JSON object for the 'values' field
  cJSON_AddNumberToObject(values, "Temp", temp);
  cJSON_AddNumberToObject(values, "Humi", humi);
  cJSON_AddNumberToObject(values, "Press", press);
  cJSON_AddNumberToObject(values, "VOC", voc);
  cJSON_AddNumberToObject(json_data, "trans_code", trans_code++);

  char *json_str = cJSON_PrintUnformatted(json_data);                 // Convert the JSON object to a string
  esp_mqtt_client_publish(client, cTopic_pub, json_str, 0, QoS_1, 0); // Publish the JSON string via MQTT
  cJSON_Delete(json_data);
  free(json_str);
}

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/