#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_system.h"
#include "esp_mac.h"
#include <string.h>
#include "esp_log.h"
#include "cJSON.h"
#include <time.h>
#include <stdio.h>
#include "esp_random.h"

#include "A7680C.h"
#include "uartdev.h"

extern QueueHandle_t uart_msg_queue;
TaskHandle_t mqtt_vPublish_task_handle = NULL;

static uint8_t trans_code;

static char cmd[512];
static char resp[512];
static char str_mac[13];

static const char *TAG = "Module sim";

void create_client_id(char *client_id)
{
  char str_rd[13];
  const char char_pool[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

  for (int i = 0; i < 12; ++i)
  {
    int idx = esp_random() % (sizeof(char_pool) - 1);
    str_rd[i] = char_pool[idx];
  }
  str_rd[12] = '\0';

  snprintf(client_id, 27, "%s%s", str_mac, str_rd);
}

void module_sim_get_mac()
{
  uint8_t mac_address[6];
  esp_efuse_mac_get_default(mac_address);
  sprintf(str_mac, "%02X%02X%02X%02X%02X%02X", mac_address[0], mac_address[1], mac_address[2], mac_address[3], mac_address[4], mac_address[5]);
}

void check_reg()
{

  snprintf(cmd, sizeof(cmd), "AT+CNBP?\r\n");
  AT_tx(cmd, 200);

  snprintf(cmd, sizeof(cmd), "AT+CPIN?\r\n");
  AT_tx(cmd, 200);

  snprintf(cmd, sizeof(cmd), "AT+CPIN?\r\n");
  AT_tx(cmd, 200);

  snprintf(cmd, sizeof(cmd), "AT+CIMI\r\n");
  AT_tx(cmd, 200);

  snprintf(cmd, sizeof(cmd), "AT+CSQ\r\n");
  AT_tx(cmd, 200);
}

void module_sim_connect_mqtt_broker()
{
  module_sim_get_mac();
  char client_id[27];
  create_client_id(client_id);

  snprintf(cmd, sizeof(cmd), "AT+CRESET\r\n");

  vTaskDelay(pdMS_TO_TICKS(3000));

  check_reg();

  snprintf(cmd, sizeof(cmd), "AT+CMQTTDISC=0,60\r\n");
  AT_tx(cmd, 500);

  snprintf(cmd, sizeof(cmd), "AT+CMQTTREL=0\r\n");
  AT_tx(cmd, 500);

  snprintf(cmd, sizeof(cmd), "AT+CMQTTSTOP\r\n");
  AT_tx(cmd, 500);

  snprintf(cmd, sizeof(cmd), "AT+CMQTTSTART\r\n");
  AT_tx(cmd, 500);

  snprintf(cmd, sizeof(cmd), "AT+CMQTTACCQ=0,\"%s\",0\r\n", client_id);
  AT_tx(cmd, 1000);

  snprintf(cmd, sizeof(cmd), "AT+CMQTTCONNECT=0,\"tcp://%s:%s\",60,1\r\n", BROKER_ADDR, BROKER_PORT);
  AT_tx(cmd, 1000);
}

void module_sim_pub_data(float temp, float humi, float press, float voc)
{
  snprintf(cmd, sizeof(cmd), "AT+CMQTTTOPIC=0,37\r\n");
  AT_tx(cmd, 500);

  snprintf(cmd, sizeof(cmd), "FIRE_DETECTION/%s/Telemetry\r\n", str_mac);
  AT_tx(cmd, 500);

  ESP_LOGI(TAG, "Pub to topic: %s", cmd);

  cJSON *json_data = cJSON_CreateObject(); // Create a JSON object for the data
  cJSON_AddStringToObject(json_data, "Device_ID", str_mac);
  cJSON_AddStringToObject(json_data, "Connect_type", "Module_Sim");
  cJSON *values = cJSON_AddObjectToObject(json_data, "Values"); // Create a nested JSON object for the 'values' field
  cJSON_AddNumberToObject(values, "Temp", temp);
  cJSON_AddNumberToObject(values, "Humi", humi);
  cJSON_AddNumberToObject(values, "Press", press);
  cJSON_AddNumberToObject(values, "VOC", voc);
  cJSON_AddNumberToObject(json_data, "trans_code", trans_code++);
  char *json_str = cJSON_PrintUnformatted(json_data); // Convert the JSON object to a string

  snprintf(cmd, sizeof(cmd), "AT+CMQTTPAYLOAD=0,%u\r\n", strlen(json_str));
  AT_tx(cmd, 500);

  snprintf(cmd, sizeof(cmd), "%s", json_str);
  AT_tx(cmd, 500);
  cJSON_Delete(json_data);
  free(json_str);

  snprintf(cmd, sizeof(cmd), "AT+CMQTTPUB=0,1,60,0\r\n");
  AT_tx(cmd, 500);
}

void module_sim_send_sms(char *text, char *phone_number)
{
  snprintf(cmd, sizeof(cmd), "AT+CSCS=\"GSM\"\r\n");
  AT_tx(cmd, 500);

  snprintf(cmd, sizeof(cmd), "AT+CMGS=\"%s\"\r\n", phone_number);
  AT_tx(cmd, 500);

  snprintf(cmd, sizeof(cmd), "%s\r\n", text);
  AT_tx(cmd, 500);
}
/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/