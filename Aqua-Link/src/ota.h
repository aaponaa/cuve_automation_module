#ifndef OTA_H
#define OTA_H

#include <Arduino.h>

void setupOTA();
void handleOTA();

void disableOTA();
bool isOTAEnabled();

#endif // OTA_H
