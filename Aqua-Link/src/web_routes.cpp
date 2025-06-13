#include "webserver.h"
#include "config.h"
#include "mqtt.h"
#include "settings.h"
#include "sensors.h"
#include "relay.h"
#include "ota.h"
#include <Preferences.h>
#include <Arduino.h>
#include <Update.h>
#include "html_gen/dashboard_html.h"
#include "html_gen/settings_html.h"

static void handleMqttStatus() {
  String json = String("{\"connected\":") + (mqtt.connected() ? "true" : "false") + "}";
  server.send(200, "application/json", json);
}

static void handleData() {
  float distance = measureDistance();
  float water_height = max(0.0f, tank_height_cm - distance);
  float percent = (eau_max_cm > 0) ? (water_height / eau_max_cm * 100) : 0;
  float volume_liters;
  if (tank_shape == 1) {
    // Cylindrical
    volume_liters = (PI * pow(tank_diameter_cm / 2, 2) * water_height) / 1000.0;
  } else {
    // Rectangular
    volume_liters = (tank_length_cm * tank_width_cm * water_height) / 1000.0;
  }

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
  json += "}";
  server.send(200, "application/json", json);
}


void handleFirmwareUpload() {
  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    Serial.printf("Update: %s\n", upload.filename.c_str());
    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (Update.end(true)) {
      Serial.printf("Update Success: %u bytes\nRebooting...\n", upload.totalSize);
    } else {
      Update.printError(Serial);
    }
  }
}

static void handleReboot() {
  server.send(200, "text/plain", "Rebooting...");
  delay(1000);
  ESP.restart();
}

// Web HTML
static void handleRoot() {
  String html = DASHBOARD_HTML;
  server.send(200, "text/html", html);
}

static void handleSettings() {
  String html = SETTINGS_HTML;
  server.send(200, "text/html", html);
}

static void handleFirmwarePage() {
  String html = R"rawliteral(
    <!DOCTYPE html>
    <html>
    <head><meta charset="UTF-8"><title>Firmware Upload</title></head>
    <body>
      <h2>Upload new firmware</h2>
      <form method="POST" action="/upload" enctype="multipart/form-data">
        <input type="file" name="update">
        <input type="submit" value="Update">
      </form>
    </body>
    </html>
  )rawliteral";
  server.send(200, "text/html", html);
}

void initWebServer() {
  // Web server setup
  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.on("/datasettings", HTTP_GET, handleDataSettings);
  server.on("/factory_reset", handleFactoryReset);
  server.on("/settings", HTTP_GET, handleSettings);
  server.on("/mqtt_status", HTTP_GET, handleMqttStatus);
  server.on("/calibrate", HTTP_POST, handleCalibrate);
  server.on("/save_settings", HTTP_POST, handleSaveSettings);
  server.on("/mqtt_connect", HTTP_POST, handleMqttConnect);
  server.on("/topics", HTTP_POST, handleTopicsSave);
  server.on("/temp_settings", HTTP_POST, saveTempSettings);
  server.on("/relay_toggle", HTTP_POST, toggleRelay);
  server.on("/ota", []() {if (!isOTAEnabled()) {setupOTA();} server.send(200, "text/plain", "OTA mode enabled. You can now upload new firmware.");});
  server.on("/disable_mqtt", HTTP_POST, disableMQTT);
  server.on("/reboot", HTTP_POST, []() {
    server.send(200, "text/html", 
      "<html><head><meta http-equiv='refresh' content='2; url=/'></head>"
      "<body><h1>Rebooting...</h1></body></html>");
    delay(500);
    ESP.restart();
  });
  server.on("/firmware", HTTP_GET, handleFirmwarePage);
  server.on("/upload", HTTP_POST, []() {
    server.send(200, "text/html", 
      "<html><head><meta http-equiv='refresh' content='2; url=/'></head>"
      "<body><h1>Update complete. Rebooting...</h1></body></html>");
    delay(500); 
    ESP.restart();
  }, handleFirmwareUpload);
  server.on("/topics_config", HTTP_GET, []() {
    String json = "[";
    for (int i = 0; i < NUM_TOPICS; ++i) {
      if (i > 0) json += ",";
      json += "{\"topic\":\"" + tank_name + "/" + topicOptions[i].topic_suffix + "\",";
      json += "\"suffix\":\"" + String(topicOptions[i].topic_suffix) + "\",";
      json += "\"enabled\":" + String(topicOptions[i].enabled ? "true" : "false") + "}";
    }
    json += "]";
    server.send(200, "application/json", json);
  });
  server.begin();
  Serial.println("✅ Web server started");
}