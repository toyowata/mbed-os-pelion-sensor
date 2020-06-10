/*******************************************************************************
 * Copyright (c) 2014, 2015 IBM Corp.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v10.html
 * and the Eclipse Distribution License is available at
 *   http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Ian Craggs - initial API and implementation and/or initial documentation
 *    Ian Craggs - make sure QoS2 processing works, and add device headers
 *******************************************************************************/

 /**
  This is a sample program to illustrate the use of the MQTT Client library
  on the mbed platform.  The Client class requires two classes which mediate
  access to system interfaces for networking and timing.  As long as these two
  classes provide the required public programming interfaces, it does not matter
  what facilities they use underneath. In this program, they use the mbed
  system libraries.

 */

#define MQTTCLIENT_QOS1 0
#define MQTTCLIENT_QOS2 0

#include "mbed.h"
#include "NTPClient.h"
#include "TLSSocket.h"
#include "MQTTClientMbedOs.h"
#include "MQTT_server_setting.h"
#include "mbed-trace/mbed_trace.h"
#include "mbed_events.h"
#include "mbedtls/error.h"

#define LED_ON  MBED_CONF_APP_LED_ON
#define LED_OFF MBED_CONF_APP_LED_OFF

volatile bool isPublish = false;
volatile bool isButtonClicked = false;

extern char json[100];
/* Flag to be set when received a message from the server. */
static volatile bool isMessageArrived = false;
/* Buffer size for a receiving message. */
const int MESSAGE_BUFFER_SIZE = 256;
/* Buffer for a receiving message. */
char messageBuffer[MESSAGE_BUFFER_SIZE];
extern NetworkInterface* network;
extern int button_count;

/*
 * Callback function called when a message arrived from server.
 */
void messageArrived(MQTT::MessageData& md)
{
    // Copy payload to the buffer.
    MQTT::Message &message = md.message;
    if(message.payloadlen >= MESSAGE_BUFFER_SIZE) {
        // TODO: handling error
    } else {
        memcpy(messageBuffer, message.payload, message.payloadlen);
    }
    messageBuffer[message.payloadlen] = '\0';

    isMessageArrived = true;
}


void mqtt_thread(void)
{    
    const float version = 1.0;
    bool isSubscribed = false;
    TLSSocket *socket = new TLSSocket; // Allocate on heap to avoid stack overflow.
    MQTTClient* mqttClient = NULL;

    DigitalOut led(MBED_CONF_APP_LED_PIN, LED_ON);

    printf("Opening network interface...\r\n");
    {
        if (network == NULL) {
            network = NetworkInterface::get_default_instance();
        }

        if (!network) {
            printf("Error! No network inteface found.\n");
            ThisThread::sleep_for(-1);
        }

        printf("Connecting to network\n");
        nsapi_connection_status_t ret = network->get_connection_status();
        if (ret == NSAPI_STATUS_DISCONNECTED) {
            nsapi_error_t err_code = network->connect();
            if (err_code) {
                printf("Unable to get connection status! Error code %d\n", ret);
                ThisThread::sleep_for(-1);
            }
        }
    }
    printf("Network interface opened successfully.\r\n");
    printf("\r\n");

    // sync the real time clock (RTC)
    NTPClient ntp(network);
    ntp.set_server("time.google.com", 123);
    time_t now = ntp.get_timestamp();
    now += (3600 * 9); // Adjust to JST timezone
    set_time(now);
    printf("Time is now %s", ctime(&now));

    printf("[AWS] Connecting to host %s:%d ...\r\n", MQTT_SERVER_HOST_NAME, MQTT_SERVER_PORT);
    {
        nsapi_error_t ret = socket->open(network);
        if (ret != NSAPI_ERROR_OK) {
            printf("Could not open socket! Error code %d\n", ret);
            ThisThread::sleep_for(-1);
        }
        ret = socket->set_root_ca_cert(SSL_CA_PEM);
        if (ret != NSAPI_ERROR_OK) {
            printf("Could not set ca cert! Error code %d\n", ret);
            ThisThread::sleep_for(-1);
        }
        ret = socket->set_client_cert_key(SSL_CLIENT_CERT_PEM, SSL_CLIENT_PRIVATE_KEY_PEM);
        if (ret != NSAPI_ERROR_OK) {
            printf("Could not set keys! Error code %d\n", ret);
            ThisThread::sleep_for(-1);
        }
        ret = socket->connect(MQTT_SERVER_HOST_NAME, MQTT_SERVER_PORT);
        if (ret != NSAPI_ERROR_OK) {
            printf("Could not connect! Error code %d\n", ret);
            ThisThread::sleep_for(-1);
        }
    }
    printf("[AWS] Connection established.\r\n");

    printf("[AWS] MQTT client is trying to connect the server ...\r\n");
    {
        MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
        data.MQTTVersion = 3;
        data.clientID.cstring = (char *)MQTT_CLIENT_ID;

        mqttClient = new MQTTClient(socket);
        int rc = mqttClient->connect(data);
        if (rc != MQTT::SUCCESS) {
            printf("[AWS] ERROR: rc from MQTT connect is %d\r\n", rc);
            ThisThread::sleep_for(-1);
        }
    }
    printf("[AWS] Client connected.\r\n");
    printf("\r\n");
    
    printf("[AWS] Client is trying to subscribe a topic \"%s\".\r\n", MQTT_TOPIC_CMD);
    {
        int rc = mqttClient->subscribe(MQTT_TOPIC_CMD, MQTT::QOS0, messageArrived);
        if (rc != MQTT::SUCCESS) {
            printf("[AWS] ERROR: rc from MQTT subscribe is %d\r\n", rc);
        }
        isSubscribed = true;
    }
    printf("[AWS] Client has subscribed a topic \"%s\".\r\n", MQTT_TOPIC_CMD);
    
    isPublish = false;
    
    printf("[AWS] To send a packet, push the button on your board.\r\n\r\n");

    // Turn off the LED to let users know connection process done.
    led = LED_OFF;

    while(1) {
        /* Check connection */
        if(!mqttClient->isConnected()){
            break;
        }
        /* Pass control to other thread. */
        if(mqttClient->yield() != MQTT::SUCCESS) {
            break;
        }
        /* Received a control message. */
        if(isMessageArrived) {
            isMessageArrived = false;
            // Just print it out here.
            printf("[AWS] Message arrived:\r\n%s\r\n", messageBuffer);
        }
        /* Publish data */
        if(isPublish) {
            isPublish = false;
            printf("%s\n",json);
            static unsigned short id = 0;

            // When sending a message, LED lights blue.
            led = LED_ON;

            MQTT::Message message;
            message.retained = false;
            message.dup = false;

            const size_t buf_size = 100;
            char *buf = new char[buf_size];
            message.payload = (void*)buf;

            message.qos = MQTT::QOS0;
            message.id = id++;
            int ret = snprintf(buf, buf_size, "%s", json);
            if(ret < 0) {
                printf("[AWS] ERROR: snprintf() returns %d.", ret);
                continue;
            }
            message.payloadlen = ret;
            // Publish a message.
            
            int rc = MQTT::FAILURE;
            if(isButtonClicked){
                isButtonClicked = false;
                rc = mqttClient->publish(MQTT_TOPIC_CMD, message);
            }else{
                rc = mqttClient->publish(MQTT_TOPIC_DATA, message);
            }
            if(rc != MQTT::SUCCESS) {
                printf("[AWS] ERROR: rc from MQTT publish is %d\r\n", rc);
            }
            printf("[AWS] Message published.\r\n");
            delete[] buf;    

            ThisThread::sleep_for(200);
            led = LED_OFF;
        }
    }

    printf("The client has disconnected.\r\n");

    if(mqttClient) {
        if(isSubscribed) {
            mqttClient->unsubscribe(MQTT_TOPIC_DATA);
        }
        if(mqttClient->isConnected()) 
            mqttClient->disconnect();
        delete mqttClient;
    }
    if(socket) {
        socket->close();
    }
    if(network) {
        network->disconnect();
        // network is not created by new.
    }
}
