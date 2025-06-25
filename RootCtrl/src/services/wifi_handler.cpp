#include "WiFiManager.h"
#include "logger.h"  

void initWifi(){
  LOG_INFO("WIFI", "Initializing WiFi connection...");
  
  WiFiManager wifiManager;
  
  LOG_INFO("WIFI", "Starting WiFi AutoConnect with SSID: RootCtrl_Wifi");
  
  if (wifiManager.autoConnect("RootCtrl_Wifi")) {
    LOG_INFO("WIFI", "WiFi connected successfully!");
    LOG_INFO("WIFI", "IP Address: " + WiFi.localIP().toString());
    LOG_INFO("WIFI", "Signal strength (RSSI): " + String(WiFi.RSSI()) + " dBm");
    LOG_INFO("WIFI", "Connected to SSID: " + WiFi.SSID());
    LOG_INFO("WIFI", "Gateway: " + WiFi.gatewayIP().toString());
    LOG_INFO("WIFI", "Subnet: " + WiFi.subnetMask().toString());
    LOG_INFO("WIFI", "DNS: " + WiFi.dnsIP().toString());
  } else {
    LOG_ERROR("WIFI", "Failed to connect to WiFi!");
    LOG_ERROR("WIFI", "WiFi Manager timeout reached");
  }
}