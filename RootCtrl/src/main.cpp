#include <Arduino.h>
#include "config.h"
#include "sensors/sensors.h"
#include "sensors/relay.h"
#include "services/mqtt.h"
#include "services/web_routes.h"
#include "services/settings.h"
#include "services/wifi_handler.h"

void setup() {
  Serial.begin(115200);
  initWifi();
  initRelay();
  initSensors();
  initWebServer();
  initMqtt();
}

void loop() {
  server.handleClient();
  handleRelayLogic();
  readSensorsLoop();
  handleMqttLoop();
}
