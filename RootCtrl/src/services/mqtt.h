#ifndef MQTT_H
#define MQTT_H

#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFiClient.h>

// MQTT initialization and loop handling
void initMqtt();
void handleMqttLoop();
void mqttReconnect();
void mqttCallback(char* topic, byte* payload, unsigned int length);

// Home Assistant discovery publishing
void publishAllDiscovery(const String& tank_name);
void removeDiscovery(const String& old_tank_name);

// Individual discovery topics for Home Assistant
void publishDiscoverySensor(
  const String& tank_name,
  const char* suffix,
  const char* name,
  const char* unit,
  const char* device_class 
);
void publishDiscoveryPumpSwitch(const String& tank_name);
void publishDiscoveryMqttStatus(const String& tank_name);

// MQTT connection from web interface
void handleMqttConnect();
void disableMQTT();

#endif // MQTT_H
