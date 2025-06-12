#ifndef SENSORS_H
#define SENSORS_H

#include "config.h"

float measureDistance();

// Initialization of sensors (DHT, DS18B20, A02YYUW or SR04M)
void initSensors();

// Main sensor polling function (called regularly in loop)
void readSensorsLoop();

// Ultrasonic distance measurement (A02YYUW or SR04M, depending on flag)
float readWaterDistance();

// Low-level ultrasonic sensor functions
float readSR04M();
float readA02YYUW();

// Temperature sensors
void readDHT();
void readDS18B20();

void handleCalibrate();

#endif
