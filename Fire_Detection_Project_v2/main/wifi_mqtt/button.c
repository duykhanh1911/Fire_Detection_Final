/*****************************************************************************
 *
 * @file 	bee_button.c
 * @author 	tuha
 * @date 	3 July 2023
 * @brief	module for project cotrol by button
 *
 ***************************************************************************/

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

#include "wifi_mqtt/button.h"
#include "wifi_mqtt/wifi.h"
#include "wifi_mqtt/nvs.h"
#include "wifi_mqtt/mqtt.h"

/****************************************************************************/
/***        Global Variables                                              ***/
/****************************************************************************/
extern bool bInit;
bool bButton_task = false;

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/

static bool button_pressed = false;
static TickType_t button_press_time = 0;
static TickType_t current_time = 0;
static const char *TAG = "BUTTON";

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

static void IRAM_ATTR gpio_isr_handler(void *arg)
{
  TickType_t last_button_press_time = 0;
  TickType_t current_time = xTaskGetTickCount();

  // Check if the current button press is too close to the previous one (debouncing)
  if ((current_time - last_button_press_time) < pdMS_TO_TICKS(50))
  {
    return; // Ignore the button press
  }
  last_button_press_time = current_time;

  if (!bButton_task)
  {
    xTaskCreate(button_task, "button_task", 8192, NULL, 10, NULL); // Create a new task for handling the button press
  }
}

void button_task(void *arg)
{
  bButton_task = true;
  ESP_LOGI(TAG, "Enter Prov WiFi Mode\n");
  wifi_prov();
  vTaskDelete(NULL);
}

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

void button_init(int gpio_num)
{
  gpio_config_t io_conf = {};
  io_conf.intr_type = GPIO_INTR_POSEDGE;
  io_conf.pin_bit_mask = (1ULL << gpio_num);
  io_conf.mode = GPIO_MODE_INPUT;
  io_conf.pull_up_en = 1;
  gpio_config(&io_conf);

  gpio_set_pull_mode(gpio_num, GPIO_PULLUP_ONLY);
  gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
  gpio_isr_handler_add(gpio_num, gpio_isr_handler, NULL);
}

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/