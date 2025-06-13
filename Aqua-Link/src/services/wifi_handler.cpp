#include "WiFiManager.h"

void initWifi(){
  WiFiManager wifiManager;
  wifiManager.autoConnect("Tank_Automation_Config");
  Serial.println("WiFi connected: ");
  Serial.println(WiFi.localIP());
}