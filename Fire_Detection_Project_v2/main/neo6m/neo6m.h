#ifndef NEO6M_H
#define NEO6M_H

#include <stdint.h>
#include <stdbool.h>
#include <esp_err.h>
#include "driver/gpio.h"
typedef struct
{
  float latitude;
  float longitude;
} gps_coordinates_t;

// Khởi tạo UART và GPS
esp_err_t neo6m_init();
esp_err_t neo6m_get_coordinates(gps_coordinates_t *results);
#endif // NEO6M_H
