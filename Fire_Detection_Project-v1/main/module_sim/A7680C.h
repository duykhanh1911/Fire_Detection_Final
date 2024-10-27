#ifndef __A7680C_H__
#define __A7680C_H__

#include "driver/gpio.h"

#define BROKER_ADDR "broker.hivemq.com"
#define BROKER_PORT "1883"

int module_sim_task_init();

void module_sim_connect_mqtt_broker();

void module_sim_pub_data(float temp, float humi, float press, float voc);

void module_sim_send_sms(char *text, char *phone_number);

#endif // __A7680C_H__