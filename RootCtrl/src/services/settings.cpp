#include "settings.h"
#include "../config.h"
#include "../services/mqtt.h"  


void initSettings() {
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
}

void saveSettings() {
  String old_name = tank_name;

  if (server.hasArg("tank_name")) tank_name = server.arg("tank_name");
  if (server.hasArg("tank_height")) tank_height_cm = server.arg("tank_height").toFloat();
  if (server.hasArg("tank_diameter")) tank_diameter_cm = server.arg("tank_diameter").toFloat();
  if (server.hasArg("tank_length")) tank_length_cm = server.arg("tank_length").toFloat();
  if (server.hasArg("tank_width")) tank_width_cm = server.arg("tank_width").toFloat();
  if (server.hasArg("eau_max")) eau_max_cm = server.arg("eau_max").toFloat();
  if (server.hasArg("tank_shape")) tank_shape = server.arg("tank_shape").toInt();

  prefs.begin("cuve", false);
  prefs.putString("tank_name", tank_name);
  prefs.putFloat("tank_height", tank_height_cm);
  prefs.putFloat("tank_diam", tank_diameter_cm);
  prefs.putFloat("tank_len", tank_length_cm);
  prefs.putFloat("tank_width", tank_width_cm);
  prefs.putFloat("eau_max", eau_max_cm);
  prefs.putBool("tank_shape", tank_shape);
  prefs.end();

  if (old_name != tank_name) {
    removeDiscovery(old_name);         
    publishAllDiscovery(tank_name);    
  }

  server.sendHeader("Location", "/settings");
  server.send(303);
}

void savePeriodSettings() {
  if (server.hasArg("mqtt_publish") && server.hasArg("temp_refresh")) {
    unsigned int mqtt_publish_sec = server.arg("mqtt_publish").toInt();
    unsigned int temp_refresh_sec = server.arg("temp_refresh").toInt();

    mqtt_publish_ms = mqtt_publish_sec * 1000;
    temp_refresh_ms = temp_refresh_sec * 1000;

    prefs.begin("cuve", false);
    prefs.putUInt("mqtt_publish", mqtt_publish_ms);
    prefs.putUInt("temp_refresh", temp_refresh_ms);
    prefs.end();
  }

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

