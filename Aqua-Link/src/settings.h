#ifndef SETTINGS_H
#define SETTINGS_H

#include <Arduino.h>
#include <Preferences.h>

void initSettings();
void initTempSettings();
void saveTempSettings();
void saveSettings();
void handleFactoryReset();

#endif
