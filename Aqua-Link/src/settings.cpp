#include "settings.h"
#include "config.h"

void initSettings() {
  prefs.begin("cuve", true);

  tank_name = prefs.getString("tank_name", "PimentosTank");
  tank_height_cm = prefs.getFloat("tank_height", 100.0);
  tank_diameter_cm = prefs.getFloat("tank_diam", 60.0);
  tank_length_cm = prefs.getFloat("tank_len", 80.0);
  tank_width_cm = prefs.getFloat("tank_width", 60.0);
  eau_max_cm = prefs.getFloat("eau_max", 100.0);
  tank_shape = prefs.getBool("tank_shape", false);

  prefs.end();
}

void initTempSettings() {
  prefs.begin("cuve", true);

  temp_refresh_ms = prefs.getUInt("temp_refresh", 2000);
  dht_refresh_ms = prefs.getUInt("dht_refresh", 10000);

  prefs.end();
}

void saveSettings() {
  prefs.begin("cuve", false);

  prefs.putString("tank_name", tank_name);
  prefs.putFloat("tank_height", tank_height_cm);
  prefs.putFloat("tank_diam", tank_diameter_cm);
  prefs.putFloat("tank_len", tank_length_cm);
  prefs.putFloat("tank_width", tank_width_cm);
  prefs.putFloat("eau_max", eau_max_cm);
  prefs.putBool("tank_shape", tank_shape);

  prefs.end();
}

void saveTempSettings() {
  prefs.begin("cuve", false);

  prefs.putUInt("temp_refresh", temp_refresh_ms);
  prefs.putUInt("dht_refresh", dht_refresh_ms);

  prefs.end();
}

void handleFactoryReset() {
  // Clear NVS partition "cuve"
  prefs.begin("cuve", false);
  prefs.clear();
  prefs.end();

  // Clear WiFi config
  WiFi.disconnect(true, true);

  server.send(200, "text/html", "<html><body><h2>Device has been reset.<br>Restarting...</h2></body></html>");
  delay(1500);
  ESP.restart();
}

