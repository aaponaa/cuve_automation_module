#define MQTT_MAX_PACKET_SIZE 1024  // Increase from default 256 to 512
#include <PubSubClient.h>
#include "mqtt.h"
#include "../sensors/relay.h"
#include "../config.h"
#include "../sensors/sensors.h"
#include <Preferences.h>

extern PubSubClient mqtt;
extern String tank_name;
extern String fw_version;

static String last_tank_name = "";

static String getDiscoveryTopic(const char* component, const String& object_id) {
    return "homeassistant/" + String(component) + "/" + object_id + "/config";
}

static String getObjectId(const String& tank_name, const char* suffix) {
    String out = tank_name;
    out.replace(" ", "_");
    return out + "_" + suffix;
}

// ----- Publish Discovery Configs -----

void publishDiscoverySensor(const String& tank_name, const char* suffix, const char* name, const char* unit, const char* device_class = nullptr) {
  String object_id = getObjectId(tank_name, suffix);
  String stateTopic = tank_name + "/" + suffix; 
  stateTopic.replace(" ", "_");
  String devId = tank_name + "_device";
  String topic = "homeassistant/sensor/" + object_id + "/config";
  
  // Build payload using String concatenation (same method that worked in simple test)
  String jsonPayload = "{";
  jsonPayload += "\"name\":\"" + String(name) + "\",";
  jsonPayload += "\"state_topic\":\"" + stateTopic + "\",";
  jsonPayload += "\"expire_after\":60,";
  jsonPayload += "\"unit_of_measurement\":\"" + String(unit) + "\",";
  
  if (device_class) {
    jsonPayload += "\"device_class\":\"" + String(device_class) + "\",";
  }
  
  jsonPayload += "\"unique_id\":\"" + object_id + "\",";
  jsonPayload += "\"device\":{";
  jsonPayload += "\"identifiers\":[\"" + devId + "\"],";
  jsonPayload += "\"name\":\"" + tank_name + "\",";
  jsonPayload += "\"manufacturer\":\"RootCtrl\",";
  jsonPayload += "\"model\":\"Water Tank Monitor\",";
  jsonPayload += "\"sw_version\":\"" + fw_version + "\"";
  jsonPayload += "}}";

  // Use the same publish method that worked in simple test
  mqtt.publish(topic.c_str(), jsonPayload.c_str(), true);


}

void publishDiscoveryPumpSwitch(const String& tank_name) {
  String object_id = getObjectId(tank_name, "pump");
  String stateTopic = tank_name + "/pump_state"; 
  stateTopic.replace(" ", "_");
  String commandTopic = tank_name + "/pump_cmd"; 
  commandTopic.replace(" ", "_");
  String devId = tank_name + "_device";

  String topic = "homeassistant/switch/" + object_id + "/config";
  
  // Build JSON payload using String concatenation (same as working method)
  String jsonPayload = "{";
  jsonPayload += "\"name\":\"Pump\",";
  jsonPayload += "\"state_topic\":\"" + stateTopic + "\",";
  jsonPayload += "\"expire_after\":60,";
  jsonPayload += "\"command_topic\":\"" + commandTopic + "\",";
  jsonPayload += "\"payload_on\":\"on\",";
  jsonPayload += "\"payload_off\":\"off\",";
  jsonPayload += "\"unique_id\":\"" + object_id + "\",";
  jsonPayload += "\"device_class\":\"switch\",";
  jsonPayload += "\"device\":{";
  jsonPayload += "\"identifiers\":[\"" + devId + "\"],";
  jsonPayload += "\"name\":\"" + tank_name + "\",";
  jsonPayload += "\"manufacturer\":\"RootCtrl\",";
  jsonPayload += "\"model\":\"Water Tank Monitor\",";
  jsonPayload += "\"sw_version\":\"" + fw_version + "\"";
  jsonPayload += "}}";
  
  mqtt.publish(topic.c_str(), jsonPayload.c_str(), true);

}

void publishDiscoveryMqttStatus(const String& tank_name) {
  String object_id = getObjectId(tank_name, "mqtt_status");
  String stateTopic = tank_name + "/mqtt_status"; 
  stateTopic.replace(" ", "_");
  String devId = tank_name + "_device";

  String topic = "homeassistant/binary_sensor/" + object_id + "/config";
  
  String jsonPayload = "{";
  jsonPayload += "\"name\":\"MQTT Status\",";
  jsonPayload += "\"state_topic\":\"" + stateTopic + "\",";
  jsonPayload += "\"expire_after\":60,";
  jsonPayload += "\"payload_on\":\"online\",";
  jsonPayload += "\"payload_off\":\"offline\",";
  jsonPayload += "\"device_class\":\"connectivity\",";
  jsonPayload += "\"unique_id\":\"" + object_id + "\",";
  jsonPayload += "\"device\":{";
  jsonPayload += "\"identifiers\":[\"" + devId + "\"],";
  jsonPayload += "\"name\":\"" + tank_name + "\",";
  jsonPayload += "\"manufacturer\":\"RootCtrl\",";
  jsonPayload += "\"model\":\"Water Tank Monitor\",";
  jsonPayload += "\"sw_version\":\"" + fw_version + "\"";
  jsonPayload += "}}";

  mqtt.publish(topic.c_str(), jsonPayload.c_str(), true);
}

// ---- Remove Discovery (on tank name change) ----
void publishAllDiscovery(const String& tank_name) {
  publishDiscoverySensor(tank_name, "water_level", "Water Level", "cm", "distance");
  publishDiscoverySensor(tank_name, "water_height", "Water Height", "cm");
  publishDiscoverySensor(tank_name, "water_percent", "Water %", "%");
  publishDiscoverySensor(tank_name, "water_volume", "Water Volume", "L");
  publishDiscoverySensor(tank_name, "water_temp", "Water Temp", "°C", "temperature");
  publishDiscoverySensor(tank_name, "outside_temp", "Outside Temp", "°C", "temperature");
  publishDiscoverySensor(tank_name, "outside_humi", "Outside Humidity", "%", "humidity");
  publishDiscoverySensor(tank_name, "water_distance", "Water Distance", "cm");

  publishDiscoveryPumpSwitch(tank_name);
  publishDiscoveryMqttStatus(tank_name);
}



// ---- Publish all entities (call on startup, after MQTT connect, or after tank name change) ----
void removeDiscovery(const String& old_tank_name) {
  String id = old_tank_name; id.replace(" ", "_");

  mqtt.publish(("homeassistant/sensor/" + id + "_water_level/config").c_str(), "", true);
  mqtt.publish(("homeassistant/sensor/" + id + "_water_height/config").c_str(), "", true);
  mqtt.publish(("homeassistant/sensor/" + id + "_water_percent/config").c_str(), "", true);
  mqtt.publish(("homeassistant/sensor/" + id + "_water_volume/config").c_str(), "", true);
  mqtt.publish(("homeassistant/sensor/" + id + "_water_temp/config").c_str(), "", true);
  mqtt.publish(("homeassistant/sensor/" + id + "_outside_temp/config").c_str(), "", true);
  mqtt.publish(("homeassistant/sensor/" + id + "_outside_humi/config").c_str(), "", true);
  mqtt.publish(("homeassistant/sensor/" + id + "_water_distance/config").c_str(), "", true);
  mqtt.publish(("homeassistant/switch/" + id + "_pump/config").c_str(), "", true);
  mqtt.publish(("homeassistant/binary_sensor/" + id + "_mqtt_status/config").c_str(), "", true);
}


// ========== Main MQTT Client Functions ==========

void initMqtt() {
    prefs.begin("cuve", true);
    mqtt_enabled = prefs.getBool("mqtt_enabled", false);
    mqtt_host = prefs.getString("mqtt_host", "");
    mqtt_username = prefs.getString("mqtt_user", "");
    mqtt_password = prefs.getString("mqtt_pass", "");
    prefs.end();

    mqtt.setServer(mqtt_host.c_str(), 1883);
    mqtt.setCallback(mqttCallback);
    mqtt.setBufferSize(1024);

    last_tank_name = tank_name;
}

void mqttReconnect() {
    if (!mqtt_enabled || mqtt.connected()) return;

    String clientId = "lient-" + tank_name;
    mqtt.setServer(mqtt_host.c_str(), 1883);
    if (mqtt.connect(clientId.c_str(), mqtt_username.c_str(), mqtt_password.c_str())) {
        Serial.println("✅ MQTT connected");

        publishAllDiscovery(tank_name);

        String prefix = tank_name; prefix.replace(" ", "_");
        mqtt.subscribe((prefix + "/pump_cmd").c_str());
        Serial.println("✅ Subscribed to: " + prefix + "/pump_cmd");

        // Publish MQTT status
        String mqtt_status_topic = prefix + "/mqtt_status";
        mqtt.publish(mqtt_status_topic.c_str(), "online", true);
    } else {
        Serial.println("❌ MQTT connection failed");
    }
}

void handleMqttLoop() {
    if (!mqtt_enabled) return;
    mqttReconnect();
    mqtt.loop();

    static unsigned long lastMqttPublish = 0;
    if (millis() - lastMqttPublish > mqtt_publish_ms) {
        lastMqttPublish = millis();

        float distance = measureDistance();
        float water_height = max(0.0f, tank_height_cm - distance);
        float percent = (eau_max_cm > 0) ? (water_height / eau_max_cm * 100) : 0;
        float volume_liters = calculateVolumeLiters(water_height);

        String prefix = tank_name; prefix.replace(" ", "_");

        mqtt.publish((prefix + "/water_distance").c_str(), String(distance, 1).c_str(), true); // ajouté

        mqtt.publish((prefix + "/water_height").c_str(), String(water_height, 1).c_str(), true);
        mqtt.publish((prefix + "/water_percent").c_str(), String(percent, 1).c_str(), true);
        mqtt.publish((prefix + "/water_volume").c_str(), String(volume_liters, 1).c_str(), true);

        mqtt.publish((prefix + "/water_temp").c_str(), String(current_temp, 1).c_str(), true);
        mqtt.publish((prefix + "/outside_temp").c_str(), String(isnan(outside_temp) ? -99 : outside_temp, 1).c_str(), true);
        mqtt.publish((prefix + "/outside_humi").c_str(), String(isnan(outside_humi) ? -1 : outside_humi, 1).c_str(), true);

        mqtt.publish((prefix + "/pump_state").c_str(), pump_is_on ? "on" : "off", true);
        mqtt.publish((prefix + "/mqtt_status").c_str(), "online", true);
    }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
    String top = String(topic);
    String prefix = tank_name; prefix.replace(" ", "_");

    if (top == prefix + "/pump_cmd") {
        String cmd = "";
        for (unsigned int i = 0; i < length; i++) cmd += (char)payload[i];
        cmd.trim();
        if (cmd.equalsIgnoreCase("on")) setRelay(true);
        else if (cmd.equalsIgnoreCase("off")) setRelay(false);
    }
}

void handleMqttConnect() {
    String clientId = "lient-" + tank_name;

    if (server.hasArg("mqtt_host")) mqtt_host = server.arg("mqtt_host");
    if (server.hasArg("mqtt_user")) mqtt_username = server.arg("mqtt_user");
    if (server.hasArg("mqtt_pass")) mqtt_password = server.arg("mqtt_pass");

    mqtt.setServer(mqtt_host.c_str(), 1883);
    bool success = mqtt.connect(clientId.c_str(), mqtt_username.c_str(), mqtt_password.c_str());

    prefs.begin("cuve", false);
    if (success) {
        prefs.putBool("mqtt_enabled", true);
        mqtt_enabled = true;
        prefs.putString("mqtt_host", mqtt_host);
        prefs.putString("mqtt_user", mqtt_username);
        prefs.putString("mqtt_pass", mqtt_password);
    } else {
        prefs.putBool("mqtt_enabled", false);
        mqtt_enabled = false;
    }
    prefs.end();

    String prefix = tank_name; prefix.replace(" ", "_");
    mqtt.subscribe((prefix + "/pump_cmd").c_str());
    
    // On connection, (re-)publish discovery info
    publishAllDiscovery(tank_name);

    // MQTT status as binary_sensor
    mqtt.publish((prefix + "/mqtt_status").c_str(), "online", true);

    String json = String("{\"connected\":") + (mqtt.connected() ? "true" : "false") + "}";
    server.send(200, "application/json", json);
}

void disableMQTT() {
    prefs.begin("cuve", false);
    prefs.putBool("mqtt_enabled", false);
    prefs.end();

    mqtt_enabled = false;
    mqtt.disconnect();
    server.send(200, "text/plain", "MQTT has been disabled");
}
