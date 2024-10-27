#include <string.h>
#include "esp_log.h"
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "bme680.h"
#include "i2cdev.h"

#include "wifi_mqtt/mqtt.h"
#include "module_sim/A7680C.h"
#include "relay/relay.h"

bme680_t sensor;
bme680_values_float_t bme680_values;
uint32_t duration;

TaskHandle_t bme680_TaskHandle = NULL;
TaskHandle_t pulish_data_TaskHandle = NULL;
TaskHandle_t warning_task_TaskHandle = NULL;

bool is_stable_sensor = false;

extern bool is_wifi_connect;

static const char *TAG = "BME680_task";

void init_bme680(void)
{
  memset(&sensor, 0, sizeof(bme680_t));

  ESP_ERROR_CHECK(bme680_init_desc(&sensor, BME680_I2C_ADDR, PORT, BME680_I2C_MASTER_SDA, BME680_I2C_MASTER_SCL));
  vTaskDelay(pdMS_TO_TICKS(10000));
  // init the sensor
  ESP_ERROR_CHECK(bme680_init_sensor(&sensor));
  vTaskDelay(pdMS_TO_TICKS(10000));
  // Changes the oversampling rates to 4x oversampling for temperature
  // and 2x oversampling for humidity. Pressure measurement is skipped.
  bme680_set_oversampling_rates(&sensor, BME680_OSR_4X, BME680_OSR_2X, BME680_OSR_2X);

  // Change the IIR filter size for temperature and pressure to 7.
  bme680_set_filter_size(&sensor, BME680_IIR_SIZE_7);

  // Change the heater profile 0 to 290 degree Celsius for 100 ms.
  bme680_set_heater_profile(&sensor, 0, 290, 100);
  bme680_use_heater_profile(&sensor, 0);

  // Set ambient temperature to 25 degree Celsius
  bme680_set_ambient_temperature(&sensor, 25);

  // as long as sensor configuration isn't changed, duration is constant
  bme680_get_measurement_duration(&sensor, &duration);
}

static esp_err_t get_bme680_readings(void)
{
  esp_err_t ret;
  // trigger the sensor to start one TPHG measurement cycle
  ret = bme680_force_measurement(&sensor);
  if (ret == ESP_OK)
  {
    // passive waiting until measurement results are available
    vTaskDelay(duration);

    // get the results and do something with them
    ret = bme680_get_results_float(&sensor, &bme680_values);

    return ret;
  }
  else
    return ret;
}

void bme680_task(void *parameter)
{
  esp_err_t ret;
  uint16_t stable_sensor_status = 0;

  for (;;)
  {
    ret = get_bme680_readings();
    if (ret == ESP_OK)
    {
      if (!is_stable_sensor)
        stable_sensor_status++;

      if (stable_sensor_status >= TIME_SENSOR_STABLE)
        is_stable_sensor = true;
    }
    else
    {
      ESP_LOGE(TAG, "Reading error: %d", ret);
      is_stable_sensor = false;
    }
    // Delay
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void warning_task(void *parameter)
{
  bool is_warning = false;
  for (;;)
  {
    if (is_stable_sensor)
    {
      if (bme680_values.gas_resistance <= CO2_WARNING_THRESHOLD && !is_warning)
      {
        relay_set_status_pump(RELAY_ON);
        is_warning = true;
      }
      else if (bme680_values.gas_resistance > CO2_WARNING_THRESHOLD && is_warning)
      {
        relay_set_status_pump(RELAY_OFF);
        is_warning = false;
      }

      if (is_warning)
      {
        relay_set_status_buzzer(RELAY_ON);
        vTaskDelay(pdMS_TO_TICKS(500));
        relay_set_status_buzzer(RELAY_OFF);
      }
    }
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

void publish_data_task(void *parameter)
{
  for (;;)
  {
    ESP_LOGI(TAG, "BME680 Sensor: %.2f °C, %.2f %%, %.2f hPa, %.2f KOhm\n", bme680_values.temperature, bme680_values.humidity, bme680_values.pressure, bme680_values.gas_resistance);
    if (is_stable_sensor)
    {
      if (is_wifi_connect)
        wifi_pub_data(bme680_values.temperature, bme680_values.humidity, bme680_values.pressure, bme680_values.gas_resistance);
      else
        module_sim_pub_data(bme680_values.temperature, bme680_values.humidity, bme680_values.pressure, bme680_values.gas_resistance);
    }
    // Delay
    vTaskDelay(pdMS_TO_TICKS(5000));
  }
}

int bme680_task_init()
{
  ESP_ERROR_CHECK(i2cdev_init());
  init_bme680();
  BaseType_t taskCreated = xTaskCreate(bme680_task, "bme680_task", 2048, NULL, 10, &bme680_TaskHandle);
  if (taskCreated != pdTRUE)
  {
    ESP_LOGE(TAG, "Failed to create bme680_task");
    return taskCreated;
  }

  taskCreated = xTaskCreate(publish_data_task, "publish_data_task", 4096, NULL, 10, &pulish_data_TaskHandle);
  if (taskCreated != pdTRUE)
  {
    ESP_LOGE(TAG, "Failed to create publish_data_task");
    return taskCreated;
  }

  taskCreated = xTaskCreate(warning_task, "warning_task", 2048, NULL, 10, &warning_task_TaskHandle);
  if (taskCreated != pdTRUE)
  {
    ESP_LOGE(TAG, "Failed to create warning_task");
    return taskCreated;
  }

  return taskCreated;
}
