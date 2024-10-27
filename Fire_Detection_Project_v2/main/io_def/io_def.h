#ifndef IO_DEF_H
#define IO_DEF_H

// I2C for sensor BME680
#define BME680_I2C_MASTER_SDA 6
#define BME680_I2C_MASTER_SCL 5

// UART for neo6m
#define UART_TX 20
#define UART_RX 21
#define UART_RST 23

// i/o for relay
#define PIN_NUM_BUZZER 10
#define PIN_NUM_PUMP 11

// button config wifi
#define CONFIG_WIFI_BUTTON 18

#define CO2_WARNING_THRESHOLD 100 // kOhm
#define TIME_SENSOR_STABLE 20     // sec

#endif // IO_DEF_H