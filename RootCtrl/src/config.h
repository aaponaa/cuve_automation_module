#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <Preferences.h>
#include <PubSubClient.h>
#include <WebServer.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <DHT.h>
#include <WiFiClient.h>

// ========== GENERAL SETTINGS ==========

#define FW_VERSION "v0.1.0"
extern String fw_version;

#define USE_A02YYUW false  // Set to false to use SR04M

// ========== PIN DEFINITIONS ==========

// Ultrasonic sensors
#define TRIG_PIN 17
#define ECHO_PIN 16  // Only used for SR04M

// A02YYUW
#define SENSOR_RX_PIN 26  // A02YYUW TX → ESP32 RX (SoftwareSerial not needed with HardwareSerial2)

// DS18B20
#define ONE_WIRE_BUS 18

// DHT22
#define DHTPIN 33
#define DHTTYPE DHT22

// Relay
#define RELAY_PIN 27
#define RELAY_ACTIVE_LOW true

// ========== GLOBALS ==========

// Web server and MQTT
extern WebServer server;
extern WiFiClient espClient;
extern PubSubClient mqtt;
extern Preferences prefs;

// Tank config
extern String tank_name;
extern int tank_shape;
extern float tank_height_cm;
extern float tank_length_cm;
extern float tank_width_cm;
extern float tank_diameter_cm;
extern float eau_max_cm;
extern float cuve_volume_l;

// Sensor values
extern float current_temp;
extern float outside_temp;
extern float outside_humi;
extern bool pump_is_on;

// Refresh intervals
extern unsigned long temp_refresh_ms;
extern unsigned long mqtt_publish_ms;
extern unsigned long dht_refresh_ms;

// MQTT config
extern String mqtt_host;
extern String mqtt_username;
extern String mqtt_password;
extern bool mqtt_enabled;

// ========== STRUCTS ==========

struct TopicOption {
  const char* label;
  const char* topic_suffix;
  bool enabled;
};

extern TopicOption topicOptions[];
extern const int NUM_TOPICS;

#endif
