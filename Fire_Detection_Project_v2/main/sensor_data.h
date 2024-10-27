#ifndef SENSOR_DATA_H
#define SENSOR_DATA_H

// Định nghĩa cấu trúc dữ liệu chung cho các cảm biến
typedef struct
{
  float temperature;
  float pressure;
  float humidity;
  float co2;
  float latitude;
  float longitude;
} sensor_data_t;

#endif // SENSOR_DATA_H