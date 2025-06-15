#ifndef WEB_ROUTES_H
#define WEB_ROUTES_H

#include <WebServer.h>

void initWebServer();
void handleSettings();
void handleMqttConnect();
void disableMQTT();
void handleRebootWithHtml();
void handleUploadSuccessAndReboot();

#endif