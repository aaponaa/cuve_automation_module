#include "config.h"
#include <WebServer.h>

String fw_version = FW_VERSION;

// Web/MQTT
WebServer server(80); 
WiFiClient espClient;
PubSubClient mqtt(espClient);
Preferences prefs;

// Tank Default configuration
String tank_name = "WaterTank";
int tank_shape = 0;
float tank_height_cm = 120.0;
float tank_length_cm = 50.0;
float tank_width_cm = 40.0;
float tank_diameter_cm = 0.0;
float eau_max_cm = 100.0;
float cuve_volume_l = 0.0;

// Sensors
float current_temp = 0.0;
float outside_temp = 0.0;
float outside_humi = 0.0;
bool pump_is_on = false;

// Intervals
unsigned long temp_refresh_ms = 2000;
unsigned long mqtt_publish_ms = 5000;

// MQTT
bool mqtt_enabled = true;

String mqtt_host = "192.168.1.100";      // ou ton IP/nom de broker
String mqtt_username = "user";          // ou vide
String mqtt_password = "pass";          // ou vide

