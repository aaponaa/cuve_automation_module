#define MQTT_MAX_PACKET_SIZE 1024  // Increase from default 256 to 512
#include <PubSubClient.h>
#include "mqtt.h"
#include "../sensors/relay.h"
#include "../config.h"
#include "../sensors/sensors.h"
#include <Preferences.h>
#include "logger.h"

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

  bool success = mqtt.publish(topic.c_str(), jsonPayload.c_str(), true);
    if (success) {
    LOG_INFO("MQTT", "Published successfully to: " + topic);
    } else {
    LOG_ERROR("MQTT", "Failed to publish to: " + topic);
    }


}

void publishDiscoveryTextSensor(const String& tank_name, const char* suffix, const char* name) {
  String object_id = getObjectId(tank_name, suffix);
  String stateTopic = tank_name + "/" + suffix;
  stateTopic.replace(" ", "_");
  String devId = tank_name + "_device";
  String topic = "homeassistant/sensor/" + object_id + "/config";

  String jsonPayload = "{";
  jsonPayload += "\"name\":\"" + String(name) + "\",";
  jsonPayload += "\"state_topic\":\"" + stateTopic + "\",";
  jsonPayload += "\"unique_id\":\"" + object_id + "\",";
  jsonPayload += "\"force_update\":true,";  
  jsonPayload += "\"value_template\":\"{{ value }}\",";
  jsonPayload += "\"device\":{";
  jsonPayload += "\"identifiers\":[\"" + devId + "\"],";
  jsonPayload += "\"name\":\"" + tank_name + "\",";
  jsonPayload += "\"manufacturer\":\"RootCtrl\",";
  jsonPayload += "\"model\":\"Water Tank Monitor\",";
  jsonPayload += "\"sw_version\":\"" + fw_version + "\"";
  jsonPayload += "}}";

  bool success = mqtt.publish(topic.c_str(), jsonPayload.c_str(), true);
  if (success) {
    LOG_INFO("MQTT", "Published text discovery to: " + topic);
  } else {
    LOG_ERROR("MQTT", "Failed to publish text discovery to: " + topic);
  }
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
  
  bool success = mqtt.publish(topic.c_str(), jsonPayload.c_str(), true);
    if (success) {
    LOG_INFO("MQTT", "Published successfully to: " + topic);
    } else {
    LOG_ERROR("MQTT", "Failed to publish to: " + topic);
    }
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

  bool success = mqtt.publish(topic.c_str(), jsonPayload.c_str(), true);
    if (success) {
    LOG_INFO("MQTT", "Published successfully to: " + topic);
    } else {
    LOG_ERROR("MQTT", "Failed to publish to: " + topic);
    }
}

// ---- Remove Discovery (on tank name change) ----
void publishAllDiscovery(const String& tank_name) {
  publishDiscoverySensor(tank_name, "water_height", "Water Height", "cm");
  publishDiscoverySensor(tank_name, "water_percent", "Water %", "%");
  publishDiscoverySensor(tank_name, "water_volume", "Water Volume", "L");
  publishDiscoverySensor(tank_name, "water_temp", "Water Temp", "°C", "temperature");
  publishDiscoverySensor(tank_name, "outside_temp", "Outside Temp", "°C", "temperature");
  publishDiscoverySensor(tank_name, "outside_humi", "Outside Humidity", "%", "humidity");
  publishDiscoverySensor(tank_name, "water_distance", "Water Distance", "cm");

  publishDiscoverySensor(tank_name, "wifi_signal", "WiFi Signal", "dBm", "signal_strength");
  publishDiscoveryTextSensor(tank_name, "wifi_ssid", "WiFi SSID");
  publishDiscoveryTextSensor("TestingBoard", "wifi_ip", "IP Address");


  publishDiscoveryPumpSwitch(tank_name);
  publishDiscoveryMqttStatus(tank_name);
}

// ---- Publish all entities (call on startup, after MQTT connect, or after tank name change) ----
void removeDiscovery(const String& old_tank_name) {
  String id = old_tank_name; id.replace(" ", "_");

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
    LOG_INFO("MQTT", "Initializing MQTT configuration...");
    
    // Charger la configuration depuis les préférences
    prefs.begin("cuve", true);
    mqtt_enabled = prefs.getBool("mqtt_enabled", false);
    mqtt_host = prefs.getString("mqtt_host", "");
    mqtt_username = prefs.getString("mqtt_user", "");
    mqtt_password = prefs.getString("mqtt_pass", "");
    prefs.end();
    
    LOG_INFO("MQTT", "Configuration loaded from preferences:");
    LOG_INFO("MQTT", "MQTT enabled: " + String(mqtt_enabled ? "true" : "false"));
    if (mqtt_enabled) {
        LOG_INFO("MQTT", "MQTT host: " + (mqtt_host.length() > 0 ? mqtt_host : String("not configured")));
        LOG_INFO("MQTT", "MQTT username: " + (mqtt_username.length() > 0 ? mqtt_username : String("not configured")));
        LOG_INFO("MQTT", "MQTT password: " + String(mqtt_password.length() > 0 ? "configured" : "not configured"));
    } else {
        LOG_INFO("MQTT", "MQTT is disabled - skipping host configuration details");
    }

    // Configuration du client MQTT
    if (mqtt_enabled && mqtt_host.length() > 0) {
        LOG_INFO("MQTT", "Configuring MQTT client...");
        mqtt.setServer(mqtt_host.c_str(), 1883);
        mqtt.setCallback(mqttCallback);
        mqtt.setBufferSize(1024);
        LOG_INFO("MQTT", "MQTT client configured - Host: " + mqtt_host + ":1883, Buffer: 1024 bytes");
    } else {
        LOG_INFO("MQTT", "MQTT client configuration skipped - MQTT disabled or host not configured");
    }

    last_tank_name = tank_name;
    LOG_DEBUG("MQTT", "Tank name saved for comparison: " + last_tank_name);
    
    LOG_INFO("MQTT", "MQTT initialization completed");
}

void mqttReconnect() {
    
    if (mqtt.connected()) {
        return;
    }

    LOG_INFO("MQTT", "Attempting MQTT reconnection...");
    
    String clientId = "client-" + tank_name;  // Correction de la typo
    LOG_DEBUG("MQTT", "Using client ID: " + clientId);
    
    mqtt.setServer(mqtt_host.c_str(), 1883);
    LOG_DEBUG("MQTT", "Connecting to MQTT broker: " + mqtt_host + ":1883");
    
    if (mqtt.connect(clientId.c_str(), mqtt_username.c_str(), mqtt_password.c_str())) {
        LOG_INFO("MQTT", "MQTT reconnection successful!");

        LOG_INFO("MQTT", "Publishing Home Assistant discovery information...");
        publishAllDiscovery(tank_name);

        String prefix = tank_name; 
        prefix.replace(" ", "_");
        String pump_topic = prefix + "/pump_cmd";
        mqtt.subscribe(pump_topic.c_str());
        LOG_INFO("MQTT", "Subscribed to pump command topic: " + pump_topic);

        String mqtt_status_topic = prefix + "/mqtt_status";
        mqtt.publish(mqtt_status_topic.c_str(), "online", true);
        LOG_INFO("MQTT", "Published online status to: " + mqtt_status_topic);
        
        LOG_INFO("MQTT", "MQTT reconnection process completed successfully");
    } else {
        LOG_ERROR("MQTT", "MQTT reconnection failed!");
        LOG_ERROR("MQTT", "MQTT error state: " + String(mqtt.state()));
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

        mqtt.publish((prefix + "/water_distance").c_str(), String(distance, 1).c_str(), true); 

        mqtt.publish((prefix + "/water_height").c_str(), String(water_height, 1).c_str(), true);
        mqtt.publish((prefix + "/water_percent").c_str(), String(percent, 1).c_str(), true);
        mqtt.publish((prefix + "/water_volume").c_str(), String(volume_liters, 1).c_str(), true);

        mqtt.publish((prefix + "/water_temp").c_str(), String(current_temp, 1).c_str(), true);
        mqtt.publish((prefix + "/outside_temp").c_str(), String(isnan(outside_temp) ? -99 : outside_temp, 1).c_str(), true);
        mqtt.publish((prefix + "/outside_humi").c_str(), String(isnan(outside_humi) ? -1 : outside_humi, 1).c_str(), true);

        mqtt.publish((prefix + "/pump_state").c_str(), pump_is_on ? "on" : "off", true);
        mqtt.publish((prefix + "/mqtt_status").c_str(), "online", true);

        mqtt.publish((prefix + "/wifi_signal").c_str(), String(WiFi.RSSI()).c_str(), true);
        mqtt.publish((prefix + "/wifi_ssid").c_str(), WiFi.SSID().c_str(), true);
        mqtt.publish((prefix + "/wifi_ip").c_str(), WiFi.localIP().toString().c_str(), true);

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
    LOG_INFO("MQTT", "Starting MQTT connection process...");
    
    String clientId = "client-" + tank_name;
    LOG_INFO("MQTT", "Generated client ID: " + clientId);

    // Récupérer les paramètres de connexion
    if (server.hasArg("mqtt_host")) {
        mqtt_host = server.arg("mqtt_host");
        LOG_INFO("MQTT", "Updated MQTT host: " + mqtt_host);
    }
    if (server.hasArg("mqtt_user")) {
        mqtt_username = server.arg("mqtt_user");
        LOG_INFO("MQTT", "Updated MQTT username: " + mqtt_username);
    }
    if (server.hasArg("mqtt_pass")) {
        mqtt_password = server.arg("mqtt_pass");
        LOG_INFO("MQTT", "MQTT password updated");
    }

    // Configuration du serveur MQTT
    LOG_INFO("MQTT", "Configuring MQTT server: " + mqtt_host + ":1883");
    mqtt.setServer(mqtt_host.c_str(), 1883);
    
    // Tentative de connexion
    LOG_INFO("MQTT", "Attempting to connect to MQTT broker...");
    bool success = mqtt.connect(clientId.c_str(), mqtt_username.c_str(), mqtt_password.c_str());

    // Sauvegarder la configuration selon le résultat
    prefs.begin("cuve", false);
    if (success) {
        LOG_INFO("MQTT", "MQTT connection successful!");
        prefs.putBool("mqtt_enabled", true);
        mqtt_enabled = true;
        prefs.putString("mqtt_host", mqtt_host);
        prefs.putString("mqtt_user", mqtt_username);
        prefs.putString("mqtt_pass", mqtt_password);
        LOG_INFO("MQTT", "MQTT configuration saved to preferences");
    } else {
        LOG_ERROR("MQTT", "MQTT connection failed!");
        LOG_ERROR("MQTT", "MQTT error state: " + String(mqtt.state()));
        prefs.putBool("mqtt_enabled", false);
        mqtt_enabled = false;
        LOG_INFO("MQTT", "MQTT disabled due to connection failure");
    }
    prefs.end();

    if (success) {
        // S'abonner aux commandes
        String prefix = tank_name; 
        prefix.replace(" ", "_");
        String pump_topic = prefix + "/pump_cmd";
        LOG_INFO("MQTT", "Subscribing to topic: " + pump_topic);
        mqtt.subscribe(pump_topic.c_str());
        
        // Publier les informations de découverte
        LOG_INFO("MQTT", "Publishing Home Assistant discovery information...");
        publishAllDiscovery(tank_name);

        // Publier le statut MQTT
        String status_topic = prefix + "/mqtt_status";
        LOG_INFO("MQTT", "Publishing online status to: " + status_topic);
        mqtt.publish(status_topic.c_str(), "online", true);
        
        LOG_INFO("MQTT", "MQTT setup completed successfully");
    }

    // Réponse HTTP
    String json = String("{\"connected\":") + (mqtt.connected() ? "true" : "false") + "}";
    LOG_INFO("WEB", "Sending MQTT connection status: " + String(mqtt.connected() ? "connected" : "disconnected"));
    server.send(200, "application/json", json);
    LOG_INFO("WEB", "HTTP response sent for MQTT connection attempt");
}

void disableMQTT() {
    LOG_INFO("MQTT", "Disabling MQTT connection...");
    
    // Sauvegarder la configuration
    prefs.begin("cuve", false);
    prefs.putBool("mqtt_enabled", false);
    prefs.end();
    LOG_INFO("MQTT", "MQTT disabled flag saved to preferences");

    // Mettre à jour la variable globale
    mqtt_enabled = false;
    LOG_INFO("MQTT", "Global MQTT enabled flag set to false");
    
    // Déconnecter le client MQTT
    if (mqtt.connected()) {
        LOG_INFO("MQTT", "Disconnecting from MQTT broker...");
        mqtt.disconnect();
        LOG_INFO("MQTT", "MQTT client disconnected successfully");
    } else {
        LOG_INFO("MQTT", "MQTT client was already disconnected");
    }
    
    LOG_INFO("WEB", "MQTT disabled via web interface");
    server.send(200, "text/plain", "MQTT has been disabled");
    LOG_INFO("WEB", "HTTP response sent: MQTT disabled confirmation");
}