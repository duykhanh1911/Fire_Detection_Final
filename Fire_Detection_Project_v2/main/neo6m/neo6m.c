#include "neo6m.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include <string.h>
#include <stdlib.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "neo6m/neo6m.h"
#include "io_def/io_def.h"
#include "sensor_data.h"
#define BUF_SIZE (1024)
static char buf[BUF_SIZE];
static const char *TAG = "NEO6M";
extern QueueHandle_t data_queue;
// Chuyển đổi chuỗi tọa độ từ NMEA sang kiểu float
float convert_to_decimal_float(char *nmea_coord, char direction)
{
    char degrees[3];
    strncpy(degrees, nmea_coord, (strlen(nmea_coord) == 9) ? 2 : 3);
    degrees[(strlen(nmea_coord) == 9) ? 2 : 3] = '\0';

    char minutes[10];
    strncpy(minutes, nmea_coord + ((strlen(nmea_coord) == 9) ? 2 : 3), 7);
    minutes[7] = '\0';
    float deg = atof(degrees);
    float min = atof(minutes);
    float decimal = deg + (min / 60.0);
    if (direction == 'S' || direction == 'W')
    {
        decimal = -decimal;
    }

    return decimal;
}

// Lấy tọa độ GPS từ chuỗi NMEA
esp_err_t neo6m_get_coordinates(gps_coordinates_t *results)
{
    sensor_data_t data;
    char buf[BUF_SIZE];
    memset(buf, 0, BUF_SIZE);
    int len = uart_read_bytes(UART_NUM_1, buf, BUF_SIZE, pdMS_TO_TICKS(1000));
    if (len > 0)
    {
        char *gga = strstr((const char *)buf, "$GPGGA");
        if (gga != NULL)
        {
            char lat[10], lon[11];
            strncpy(lat, gga + 17, 9);  // Vĩ độ
            strncpy(lon, gga + 30, 10); // Kinh độ
            lat[9] = '\0';
            lon[10] = '\0';
            char lat_dir = gga[27]; // Hướng vĩ độ (N hoặc S)
            char lon_dir = gga[41];
            // Chuyển đổi tọa độ sang độ thập phân float
            results->latitude = convert_to_decimal_float(lat, lat_dir);
            results->longitude = convert_to_decimal_float(lon, lon_dir);
        }
        else
        {
            ESP_LOGW(TAG, "Không tìm thấy chuỗi GPGGA trong dữ liệu nhận được");
        }
    }
    else
    {
        ESP_LOGW(TAG, "Không có dữ liệu từ GPS hoặc đọc thất bại");
    }
    return ESP_OK;
}