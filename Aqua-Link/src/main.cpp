#include <Arduino.h>
#include "config.h"
#include "sensors.h"
#include "relay.h"
#include "mqtt.h"
#include "web_routes.h"
#include "ota.h"
#include "settings.h"
#include "wifi_handler.h"

void setup() {
  Serial.begin(115200);
  initWifi();
  initSettings();
  initRelay();
  initSensors();
  initWebServer();
  initMqtt();
}

void loop() {
  //if (isOTAEnabled) {
  //  setupOTA();
  //  handleOTA();
  //} else {
  server.handleClient();
  handleRelayLogic();
  readSensorsLoop();
  handleMqttLoop();
  //}
}
