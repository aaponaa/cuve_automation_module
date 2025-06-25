#include <Arduino.h>
#include "config.h"
#include "sensors/sensors.h"
#include "sensors/relay.h"
#include "services/mqtt.h"
#include "services/web_routes.h"
#include "services/settings.h"
#include "services/wifi_handler.h"
#include "services/logger.h"

void setup() {
  Serial.begin(115200);
  
  // Initialize logger first
  Logger::getInstance().setSerialEnabled(true);
  Logger::getInstance().setMinLevel(LOG_DEBUG);
  
  LOG_INFO("SYSTEM", "Starting Water Tank Monitor " + String(FW_VERSION));
  LOG_INFO("SYSTEM", "ESP32 Chip ID: " + String((uint32_t)ESP.getEfuseMac(), HEX));
  LOG_INFO("SYSTEM", "Free heap: " + String(ESP.getFreeHeap()) + " bytes");
  
  initWifi();
  initSettings();
  initRelay();
  initSensors();
  initWebServer();
  initMqtt();
  
  LOG_INFO("SYSTEM", "Setup complete. System ready.");
}

void loop() {
  server.handleClient();
  readSensorsLoop();
  handleMqttLoop();
  
  // Log system health every 30 seconds
  static unsigned long lastHealthLog = 0;
  if (millis() - lastHealthLog > 30000) {
    LOG_DEBUG("SYSTEM", "Free heap: " + String(ESP.getFreeHeap()) + " bytes");
    LOG_DEBUG("SYSTEM", "Uptime: " + String(millis() / 1000) + " seconds");
    lastHealthLog = millis();
  }
}