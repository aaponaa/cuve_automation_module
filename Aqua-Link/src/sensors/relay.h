#ifndef RELAY_H
#define RELAY_H

#include "config.h"

// Initialize the relay pin
void initRelay();

// Toggle relay state
void toggleRelay();

// Set relay ON or OFF
void setRelay(bool on);

// Return current relay state
bool isRelayOn();

// Handle relay logic loop (if needed for automation)
void handleRelayLogic();

#endif
