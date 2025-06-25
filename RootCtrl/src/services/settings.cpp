#include "settings.h"
#include "../config.h"
#include "../services/mqtt.h"  
#include "logger.h"


void initSettings() {
  LOG_INFO("SETTINGS", "Initializing settings from preferences...");
  
  prefs.begin("cuve", true);

  tank_name = prefs.getString("tank_name", "WaterTank");
  tank_height_cm = prefs.getFloat("tank_height", 100.0);
  tank_diameter_cm = prefs.getFloat("tank_diam", 60.0);
  tank_length_cm = prefs.getFloat("tank_len", 80.0);
  tank_width_cm = prefs.getFloat("tank_width", 60.0);
  eau_max_cm = prefs.getFloat("eau_max", 100.0);
  tank_shape = prefs.getBool("tank_shape", false);
  mqtt_publish_ms = prefs.getUInt("mqtt_publish", 5000);
  temp_refresh_ms = prefs.getUInt("temp_refresh", 2000);

  prefs.end();
  
  LOG_INFO("SETTINGS", "Settings loaded successfully:");
  LOG_INFO("SETTINGS", "Tank name: " + tank_name);
  LOG_INFO("SETTINGS", "Tank height: " + String(tank_height_cm) + " cm");
  LOG_INFO("SETTINGS", "Tank shape: " + String(tank_shape ? "rectangular" : "cylindrical"));
  if (tank_shape) {
    LOG_INFO("SETTINGS", "Tank dimensions: " + String(tank_length_cm) + "x" + String(tank_width_cm) + " cm");
  } else {
    LOG_INFO("SETTINGS", "Tank diameter: " + String(tank_diameter_cm) + " cm");
  }
  LOG_INFO("SETTINGS", "Max water level: " + String(eau_max_cm) + " cm");
  LOG_INFO("SETTINGS", "MQTT publish interval: " + String(mqtt_publish_ms) + " ms");
  LOG_INFO("SETTINGS", "Temperature refresh interval: " + String(temp_refresh_ms) + " ms");
}

void saveSettings() {
  LOG_INFO("SETTINGS", "Saving tank settings via web interface...");
  
  String old_name = tank_name;
  LOG_DEBUG("SETTINGS", "Current tank name: " + old_name);

  if (server.hasArg("tank_name")) {
    tank_name = server.arg("tank_name");
    LOG_INFO("SETTINGS", "Tank name updated: " + tank_name);
  }
  if (server.hasArg("tank_height")) {
    tank_height_cm = server.arg("tank_height").toFloat();
    LOG_INFO("SETTINGS", "Tank height updated: " + String(tank_height_cm) + " cm");
  }
  if (server.hasArg("tank_diameter")) {
    tank_diameter_cm = server.arg("tank_diameter").toFloat();
    LOG_INFO("SETTINGS", "Tank diameter updated: " + String(tank_diameter_cm) + " cm");
  }
  if (server.hasArg("tank_length")) {
    tank_length_cm = server.arg("tank_length").toFloat();
    LOG_INFO("SETTINGS", "Tank length updated: " + String(tank_length_cm) + " cm");
  }
  if (server.hasArg("tank_width")) {
    tank_width_cm = server.arg("tank_width").toFloat();
    LOG_INFO("SETTINGS", "Tank width updated: " + String(tank_width_cm) + " cm");
  }
  if (server.hasArg("eau_max")) {
    eau_max_cm = server.arg("eau_max").toFloat();
    LOG_INFO("SETTINGS", "Max water level updated: " + String(eau_max_cm) + " cm");
  }
  if (server.hasArg("tank_shape")) {
    tank_shape = server.arg("tank_shape").toInt();
    LOG_INFO("SETTINGS", "Tank shape updated: " + String(tank_shape ? "rectangular" : "cylindrical"));
  }

  LOG_INFO("SETTINGS", "Saving settings to preferences...");
  prefs.begin("cuve", false);
  prefs.putString("tank_name", tank_name);
  prefs.putFloat("tank_height", tank_height_cm);
  prefs.putFloat("tank_diam", tank_diameter_cm);
  prefs.putFloat("tank_len", tank_length_cm);
  prefs.putFloat("tank_width", tank_width_cm);
  prefs.putFloat("eau_max", eau_max_cm);
  prefs.putBool("tank_shape", tank_shape);
  prefs.end();
  LOG_INFO("SETTINGS", "Settings saved to preferences successfully");

  if (old_name != tank_name) {
    LOG_INFO("SETTINGS", "Tank name changed - updating MQTT discovery");
    LOG_INFO("MQTT", "Removing old discovery for: " + old_name);
    removeDiscovery(old_name);         
    LOG_INFO("MQTT", "Publishing new discovery for: " + tank_name);
    publishAllDiscovery(tank_name);    
  }

  LOG_INFO("WEB", "Redirecting to settings page after save");
  server.sendHeader("Location", "/settings");
  server.send(303);
}

void savePeriodSettings() {
  LOG_INFO("SETTINGS", "Saving period settings via web interface...");
  
  if (server.hasArg("mqtt_publish") && server.hasArg("temp_refresh")) {
    unsigned int mqtt_publish_sec = server.arg("mqtt_publish").toInt();
    unsigned int temp_refresh_sec = server.arg("temp_refresh").toInt();

    LOG_INFO("SETTINGS", "MQTT publish period: " + String(mqtt_publish_sec) + " seconds");
    LOG_INFO("SETTINGS", "Temperature refresh period: " + String(temp_refresh_sec) + " seconds");

    mqtt_publish_ms = mqtt_publish_sec * 1000;
    temp_refresh_ms = temp_refresh_sec * 1000;

    LOG_DEBUG("SETTINGS", "Converted to milliseconds: MQTT=" + String(mqtt_publish_ms) + "ms, Temp=" + String(temp_refresh_ms) + "ms");

    LOG_INFO("SETTINGS", "Saving period settings to preferences...");
    prefs.begin("cuve", false);
    prefs.putUInt("mqtt_publish", mqtt_publish_ms);
    prefs.putUInt("temp_refresh", temp_refresh_ms);
    prefs.end();
    LOG_INFO("SETTINGS", "Period settings saved successfully");
  } else {
    LOG_WARNING("SETTINGS", "Missing period parameters in request");
  }

  LOG_INFO("WEB", "Redirecting to settings page after period save");
  server.sendHeader("Location", "/settings");
  server.send(303);
}


void handleFactoryReset() {
  prefs.begin("cuve", false);
  prefs.clear();
  prefs.end();

  WiFi.disconnect(true, true);

  server.send(200, "text/html", "<html><body><h2>Device has been reset.<br>Restarting...</h2></body></html>");
  delay(1500);
  ESP.restart();
}

