#include "settings.h"
#include "../config.h"
#include "../services/mqtt.h"  


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

  dht_refresh_ms = prefs.getUInt("dht_refresh", 10000);

  prefs.end();
}

void handleSaveSettings() {
  String old_name = tank_name;

  // Met à jour les variables globales depuis le formulaire
  if (server.hasArg("tank_name")) tank_name = server.arg("tank_name");
  if (server.hasArg("tank_height")) tank_height_cm = server.arg("tank_height").toFloat();
  if (server.hasArg("tank_diameter")) tank_diameter_cm = server.arg("tank_diameter").toFloat();
  if (server.hasArg("tank_length")) tank_length_cm = server.arg("tank_length").toFloat();
  if (server.hasArg("tank_width")) tank_width_cm = server.arg("tank_width").toFloat();
  if (server.hasArg("eau_max")) eau_max_cm = server.arg("eau_max").toFloat();
  if (server.hasArg("tank_shape")) tank_shape = server.arg("tank_shape").toInt();

  // Sauvegarde en NVS
  prefs.begin("cuve", false);
  prefs.putString("tank_name", tank_name);
  prefs.putFloat("tank_height", tank_height_cm);
  prefs.putFloat("tank_diam", tank_diameter_cm);
  prefs.putFloat("tank_len", tank_length_cm);
  prefs.putFloat("tank_width", tank_width_cm);
  prefs.putFloat("eau_max", eau_max_cm);
  prefs.putBool("tank_shape", tank_shape);
  prefs.end();

  // Si le nom de cuve a changé, met à jour MQTT Discovery
  if (old_name != tank_name) {
    removeDiscovery(old_name);          // Supprime anciens topics
    publishAllDiscovery(tank_name);     // Publie avec le nouveau nom
  }

  // Redirection vers la page settings
  server.sendHeader("Location", "/settings");
  server.send(303);
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

