// ota.cpp
#include "ota.h"
#include <ArduinoOTA.h>

bool ota_enabled = false;
unsigned long ota_start_time = 0;
const unsigned long OTA_TIMEOUT_MS = 15000;  // 15 secondes

void setupOTA() {
  ArduinoOTA.setPassword("esp32secure");
  ota_start_time = millis();  // marque le début
  ota_enabled = true;

  ArduinoOTA.onStart([]() {
    Serial.println("OTA update starting...");
  });

  ArduinoOTA.onEnd([]() {
    Serial.println("\nOTA update complete.");
    ota_enabled = false;
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("OTA Progress: %u%%\r", (progress * 100) / total);
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("OTA Error[%u]: ", error);
    ota_enabled = false;
  });

  ArduinoOTA.begin();
  Serial.println("OTA ready, waiting for update...");
}

void handleOTA() {
  if (!ota_enabled) return;

  ArduinoOTA.handle();

  // désactivation automatique après timeout
  if (millis() - ota_start_time > OTA_TIMEOUT_MS) {
    Serial.println("⏱️ OTA timeout reached, disabling OTA.");
    ota_enabled = false;
  }
}

void disableOTA() {
  ota_enabled = false;
}

bool isOTAEnabled() {
  return ota_enabled;
}
