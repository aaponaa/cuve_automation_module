#include "relay.h"

void initRelay() {
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, RELAY_ACTIVE_LOW ? HIGH : LOW); // Relay off at start
  pump_is_on = false;
}

void toggleRelay() {
  pump_is_on = !pump_is_on;
  digitalWrite(RELAY_PIN, (RELAY_ACTIVE_LOW ? !pump_is_on : pump_is_on));
}

void setRelay(bool on) {
  pump_is_on = on;
  digitalWrite(RELAY_PIN, (RELAY_ACTIVE_LOW ? !on : on));
}

bool isRelayOn() {
  return pump_is_on;
}

