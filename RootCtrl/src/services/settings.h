#ifndef SETTINGS_H
#define SETTINGS_H

#include <Arduino.h>
#include <Preferences.h>

void initSettings();
void initTempSettings();
void initTopicSettings();
void saveTempSettings();
void handleSaveSettings();
void handleFactoryReset();

#endif
