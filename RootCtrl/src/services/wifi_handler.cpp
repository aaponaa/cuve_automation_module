#include "WiFiManager.h"

void initWifi(){
  WiFiManager wifiManager;
  wifiManager.autoConnect("RootCtrl_Wifi");
  Serial.println(WiFi.localIP());
  Serial.println(WiFi.RSSI());
}