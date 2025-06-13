#include "mqtt.h"
#include "../sensors/relay.h"
#include "../config.h" 
#include "../sensors/sensors.h"
#include <Preferences.h>

extern PubSubClient mqtt;
extern String tank_name;

void initMqtt() {
  prefs.begin("cuve", true);
  mqtt_enabled = prefs.getBool("mqtt_enabled", false);
  mqtt_host = prefs.getString("mqtt_host", "");
  mqtt_username = prefs.getString("mqtt_user", "");
  mqtt_password = prefs.getString("mqtt_pass", "");
  prefs.end();

  // MQTT setup
  mqtt.setServer(mqtt_host.c_str(), 1883);
  mqtt.setCallback(mqttCallback);
}

void mqttReconnect() {
  if (!mqtt_enabled || mqtt.connected()) return;

  String clientId = "ESP32Client-" + tank_name;
  mqtt.setServer(mqtt_host.c_str(), 1883);
  if (mqtt.connect(clientId.c_str(), mqtt_username.c_str(), mqtt_password.c_str())) {
    Serial.println("✅ MQTT connected");

    publishRelayDiscovery();

    String prefix = tank_name;
    prefix.replace(" ", "_");
    mqtt.subscribe((prefix + "/pump_cmd").c_str());
    Serial.println("Subscribed to: " + prefix + "/pump_cmd");
  } else {
    Serial.println("❌ MQTT connection failed");
  }
}

void handleMqttLoop() {
if (mqtt_enabled) {
    // MQTT reconnection logic
    mqttReconnect();
    mqtt.loop();

    // Periodic MQTT publishing
    static unsigned long lastMqttPublish = 0;
    if (millis() - lastMqttPublish > mqtt_publish_ms) {
      lastMqttPublish = millis();

      float distance = measureDistance();
      float water_height = max(0.0f, tank_height_cm - distance);
      float percent = (eau_max_cm > 0) ? (water_height / eau_max_cm * 100) : 0;
      float volume_liters = calculateVolumeLiters();

      if (mqtt.connected()) {
        String prefix = tank_name;
        prefix.replace(" ", "_");
        for (int i = 0; i < NUM_TOPICS; ++i) {
          if (topicOptions[i].enabled) {
            String topic = prefix + "/" + topicOptions[i].topic_suffix;
            String val;

            if (strcmp(topicOptions[i].topic_suffix, "water_distance") == 0) val = String(distance, 1);
            else if (strcmp(topicOptions[i].topic_suffix, "water_height") == 0) val = String(water_height, 1);
            else if (strcmp(topicOptions[i].topic_suffix, "water_percent") == 0) val = String(percent, 0);
            else if (strcmp(topicOptions[i].topic_suffix, "water_volume") == 0) val = String(volume_liters, 1);
            else if (strcmp(topicOptions[i].topic_suffix, "water_temp") == 0) val = String(current_temp, 1);
            else if (strcmp(topicOptions[i].topic_suffix, "outside_temp") == 0) val = String(outside_temp, 1);
            else if (strcmp(topicOptions[i].topic_suffix, "outside_humi") == 0) val = String(outside_humi, 1);
            else if (strcmp(topicOptions[i].topic_suffix, "pump_state") == 0) val = pump_is_on ? "ON" : "OFF";

            mqtt.publish(topic.c_str(), val.c_str());
          }
        }
      }
    }
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String top = String(topic);
  if (top.endsWith("/pump_cmd")) {
    String cmd = "";
    for (unsigned int i = 0; i < length; i++) cmd += (char)payload[i];
    cmd.trim();
    if (cmd.equalsIgnoreCase("on")) setRelay(true);
    else if (cmd.equalsIgnoreCase("off")) setRelay(false);
  }
}

void publishRelayDiscovery() {
  String prefix = tank_name;
  prefix.replace(" ", "_");
  String configTopic = "homeassistant/switch/" + prefix + "/config";
  String stateTopic = prefix + "/pump_state";
  String commandTopic = prefix + "/pump_cmd";

  String payload = "{\"name\":\"Pump\","
                   "\"command_topic\":\"" + commandTopic + "\","
                   "\"state_topic\":\"" + stateTopic + "\","
                   "\"payload_on\":\"on\","
                   "\"payload_off\":\"off\","
                   "\"unique_id\":\"" + prefix + "_pump\","
                   "\"device_class\":\"switch\"}";

  mqtt.publish(configTopic.c_str(), payload.c_str(), true);
}

void handleMqttConnect() {
  String clientId = "ESP32Client-" + tank_name;

  if (server.hasArg("mqtt_host")) mqtt_host = server.arg("mqtt_host");
  if (server.hasArg("mqtt_user")) mqtt_username = server.arg("mqtt_user");
  if (server.hasArg("mqtt_pass")) mqtt_password = server.arg("mqtt_pass");

  mqtt.setServer(mqtt_host.c_str(), 1883);
  bool success = mqtt.connect(clientId.c_str(), mqtt_username.c_str(), mqtt_password.c_str());

  prefs.begin("cuve", false);
  if (success) {
    prefs.putBool("mqtt_enabled", true);
    mqtt_enabled = prefs.getBool("mqtt_enabled", true);
    prefs.putString("mqtt_host", mqtt_host);
    prefs.putString("mqtt_user", mqtt_username);
    prefs.putString("mqtt_pass", mqtt_password);
  } else {
    prefs.putBool("mqtt_enabled", false);
  }
  prefs.end();

  // Subscribe to command topic
  String prefix = tank_name;
  prefix.replace(" ", "_");
  mqtt.subscribe((prefix + "/pump_cmd").c_str());
  Serial.println("Subscribed to: " + prefix + "/pump_cmd");

  String json = String("{\"connected\":") + (mqtt.connected() ? "true" : "false") + "}";
  server.send(200, "application/json", json);
}

void disableMQTT() {
  // Save to EEPROM that MQTT is disabled
  prefs.begin("cuve", false);
  prefs.putBool("mqtt_enabled", false);
  prefs.end();

  mqtt_enabled = false;

  // Optional: Stop the MQTT client immediately
  mqtt.disconnect();

  // Respond to the request
  server.send(200, "text/plain", "MQTT has been disabled");
}

void handleTopicsSave() {
  prefs.begin("cuve", false);
  if (server.hasArg("mqtt_enabled")) {
    mqtt_enabled = true;
  } else {
    mqtt_enabled = false;
  }
  if (server.hasArg("temp_refresh")) temp_refresh_ms = server.arg("temp_refresh").toInt() * 1000;
  if (server.hasArg("mqtt_publish")) mqtt_publish_ms = server.arg("mqtt_publish").toInt() * 1000;
  prefs.putBool("mqtt_enabled", mqtt_enabled);
  prefs.putInt("temp_refresh_ms", temp_refresh_ms);
  prefs.putInt("mqtt_publish_ms", mqtt_publish_ms);
  for (int i = 0; i < NUM_TOPICS; ++i) {
    String field = String("enable_") + topicOptions[i].topic_suffix;
    if (server.hasArg(field)) {
      topicOptions[i].enabled = true;
    } else {
      topicOptions[i].enabled = false;
    }
    prefs.putBool(topicOptions[i].topic_suffix, topicOptions[i].enabled);
  }
  prefs.end();
  server.sendHeader("Location", "/settings");
  server.send(303);
}