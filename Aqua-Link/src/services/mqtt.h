#ifndef MQTT_H
#define MQTT_H

#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFiClient.h>

// Forward declarations
void initMqtt();
void handleMqttLoop();
void mqttReconnect();
void mqttCallback(char* topic, byte* payload, unsigned int length);
void publishRelayDiscovery();
void handleTopicsSave();
void disableMQTT();
void handleMqttConnect();


#endif // MQTT_H
