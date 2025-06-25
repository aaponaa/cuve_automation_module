#include "webserver.h"
#include "../services/mqtt.h"
#include "../services/settings.h"
#include "../services/logger.h"  // Ajout de l'include pour le logger
#include "../sensors/sensors.h"
#include "../sensors/relay.h"
#include <Preferences.h>
#include <Arduino.h>
#include <Update.h>
#include "html_gen/dashboard_html.h"
#include "html_gen/settings_html.h"
#include "html_gen/logs_html.h"  // Ajout de l'include pour la page logs

static void handleMqttStatus() {
  String json = String("{\"connected\":") + (mqtt.connected() ? "true" : "false") + "}";
  server.send(200, "application/json", json);
}

static void handleData() {
  float distance = measureDistance();
  float water_height = max(0.0f, tank_height_cm - distance);
  float percent = (eau_max_cm > 0) ? (water_height / eau_max_cm * 100) : 0;
  float volume_liters = calculateVolumeLiters(water_height);

  String json = "{";
  json += "\"tank_name\":\"" + tank_name + "\",";
  json += "\"fw_version\":\"" + String(fw_version) + "\",";
  json += "\"distance\":" + String(distance, 1) + ",";
  json += "\"water_height\":" + String(water_height, 1) + ",";
  json += "\"percent\":" + String(percent, 0) + ",";
  json += "\"volume_liters\":" + String(volume_liters, 1) + ",";
  json += "\"temp\":" + String(current_temp, 1) + ",";
  json += "\"outside_temp\":" + String(isnan(outside_temp) ? -99 : outside_temp, 1) + ",";
  json += "\"outside_humi\":" + String(isnan(outside_humi) ? -1 : outside_humi, 1) + ",";
  json += "\"pump_is_on\":" + String(pump_is_on ? "true" : "false");
  json += ",\"wifi_signal\":" + String(WiFi.RSSI());
  json += "}";

  server.send(200, "application/json", json);
}

static void handleDataSettings() {
  String json = "{";
  json += "\"tank_name\":\"" + tank_name + "\",";
  json += "\"tank_height_cm\":" + String(tank_height_cm, 1) + ",";
  json += "\"tank_shape\":" + String(tank_shape) + ",";
  json += "\"tank_length_cm\":" + String(tank_length_cm, 1) + ",";
  json += "\"tank_width_cm\":" + String(tank_width_cm, 1) + ",";
  json += "\"tank_diameter_cm\":" + String(tank_diameter_cm, 1) + ",";
  json += "\"mqtt_host\":\"" + mqtt_host + "\",";
  json += "\"mqtt_username\":\"" + mqtt_username + "\",";
  json += "\"mqtt_password\":\"" + mqtt_password + "\",";
  json += "\"mqtt_publish_s\":" + String(mqtt_publish_ms / 1000) + ",";
  json += "\"temp_refresh_s\":" + String(temp_refresh_ms / 1000);
  json += ",\"use_dht\":" + String(use_dht ? "true" : "false");
  json += ",\"use_ds18b20\":" + String(use_ds18b20 ? "true" : "false");
  json += ",\"ultrasonic_mode\":" + String(ultrasonic_mode);
  json += "}";
  server.send(200, "application/json", json);
}

static void handleLogsPage() {
  String html = LOGS_HTML;
  server.send(200, "text/html", html);
}

static void handleGetLogs() {
  int count = 50; 
  
  if (server.hasArg("count")) {
    count = server.arg("count").toInt();
    if (count <= 0 || count > 1000) {
      count = 50; 
    }
  }
  
  String json = Logger::getInstance().getLogsAsJson(count);
  server.send(200, "application/json", json);
}

static void handleClearLogs() {
  Logger::getInstance().clear();
  LOG_INFO("WEB", "Logs cleared via web interface");
  server.send(200, "text/plain", "Logs cleared successfully");
}


void handleFirmwareUpload() {
  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    Serial.printf("Update: %s\n", upload.filename.c_str());
    LOG_INFO("UPDATE", "Starting firmware update: " + String(upload.filename));
    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
      Update.printError(Serial);
      LOG_ERROR("UPDATE", "Failed to begin update");
    }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      Update.printError(Serial);
      LOG_ERROR("UPDATE", "Write failed");
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (Update.end(true)) {
      Serial.printf("Update Success: %u bytes\nRebooting...\n", upload.totalSize);
      LOG_INFO("UPDATE", "Update successful: " + String(upload.totalSize) + " bytes");
    } else {
      Update.printError(Serial);
      LOG_ERROR("UPDATE", "Update failed at end");
    }
  }
}

void handleRebootWithHtml() {
  LOG_INFO("WEB", "Reboot requested via web interface");
  server.send(200, "text/html",
    "<html><head><meta http-equiv='refresh' content='2; url=/'></head>"
    "<body><h1>Rebooting...</h1></body></html>");
  delay(500);
  ESP.restart();
}

void handleUploadSuccessAndReboot() {
  LOG_INFO("WEB", "Firmware upload completed, rebooting...");
  server.send(200, "text/html",
    "<html><head><meta http-equiv='refresh' content='2; url=/'></head>"
    "<body><h1>Update complete. Rebooting...</h1></body></html>");
  delay(500);
  ESP.restart();
}

static void handleRoot() {
  String html = DASHBOARD_HTML;
  server.send(200, "text/html", html);
}

static void handleSettings() {
  String html = SETTINGS_HTML;
  server.send(200, "text/html", html);
}

void initWebServer() {
  // Routes existantes
  server.on("/", HTTP_GET, handleRoot);
  server.on("/settings", HTTP_GET, handleSettings);
  server.on("/data", HTTP_GET, handleData);
  server.on("/datasettings", HTTP_GET, handleDataSettings);
  server.on("/factory_reset", handleFactoryReset);
  server.on("/mqtt_status", HTTP_GET, handleMqttStatus);
  server.on("/calibrate", HTTP_POST, handleCalibrate);
  server.on("/save_settings", HTTP_POST, saveSettings);
  server.on("/mqtt_connect", HTTP_POST, handleMqttConnect);
  server.on("/relay_toggle", HTTP_POST, toggleRelay);
  server.on("/disable_mqtt", HTTP_POST, disableMQTT);
  server.on("/period", HTTP_POST, savePeriodSettings);
  server.on("/reboot", HTTP_POST, handleRebootWithHtml);
  server.on("/upload", HTTP_POST, handleUploadSuccessAndReboot, handleFirmwareUpload);
  
  server.on("/logs", HTTP_GET, []() {
    if (!server.hasArg("count")) {
      handleLogsPage();
    } else {
      handleGetLogs();
    }
  });
  server.on("/clear_logs", HTTP_POST, handleClearLogs);   
  server.begin();
  LOG_INFO("WEB", "Web server started with logs endpoints");
}