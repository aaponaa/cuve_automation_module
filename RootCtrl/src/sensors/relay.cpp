#include "relay.h"
#include "../services/logger.h"

void initRelay() {
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, RELAY_ACTIVE_LOW ? HIGH : LOW); // Relay off at start
  pump_is_on = false;
  
  LOG_INFO("RELAY", "Relay initialized on pin " + String(RELAY_PIN) + 
           " (Active " + String(RELAY_ACTIVE_LOW ? "LOW" : "HIGH") + ")");
}

void toggleRelay() {
  pump_is_on = !pump_is_on;
  digitalWrite(RELAY_PIN, (RELAY_ACTIVE_LOW ? !pump_is_on : pump_is_on));
  
  LOG_INFO("RELAY", "Pump toggled to " + String(pump_is_on ? "ON" : "OFF"));
}

void setRelay(bool on) {
  pump_is_on = on;
  digitalWrite(RELAY_PIN, (RELAY_ACTIVE_LOW ? !on : on));
  
  LOG_INFO("RELAY", "Pump set to " + String(on ? "ON" : "OFF"));
}

bool isRelayOn() {
  return pump_is_on;
}