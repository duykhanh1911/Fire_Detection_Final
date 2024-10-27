/*****************************************************************************
 *
 * @file 	bee_mqtt.h
 * @author 	tuha
 * @date 	5 July 2023
 * @brief	module for send data through mqtt
 * @brief	and receive command from host main through mqtt
 ***************************************************************************/

/****************************************************************************/
#include <stdint.h>
#ifndef BEE_MQTT_H
#define BEE_MQTT_H

#define MAC_ADDR_SIZE 6
#define QoS_0 0
#define QoS_1 1
#define QoS_2 2

#define BROKER_ADDRESS_URI "mqtt://broker.hivemq.com:1883"

void wifi_mqtt_reconnect();

/**
 * @brief   Initialize MQTT functionality.
 *
 * This function configures and initializes the MQTT client. It sets up the MQTT broker address URI, credentials (username
 * and password), and initializes the MQTT client. It registers an event handler for MQTT events, starts the MQTT client, and
 * creates an MQTT command queue. The MAC address of the Wi-Fi station interface is retrieved to construct MQTT topics for
 * publishing and subscribing. Information about the topics and initialization progress is logged.
 *
 * @note    Make sure to customize the BROKER_ADDRESS_URI, USERNAME, and PASSWORD constants before using this function.
 */
void wifi_mqtt_func_init(void);

/**
 * @brief Publishes temperature and humidity data via MQTT.
 *
 * This function creates a JSON object containing temperature and humidity data,
 * then publishes it using the MQTT client.
 *
 * @param fTemp The temperature value to be published.
 * @param fHumi The humidity value to be published.
 */
void wifi_pub_data(float temp, float humi, float press, float voc);

#endif /* BEE_MQTT_H */

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/