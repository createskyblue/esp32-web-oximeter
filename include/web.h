#ifndef WEB_SERVER_H
#define WEB_SERVER_H
#include <Arduino.h>
#include "main.h"

#include <WebServer.h>
#include <ArduinoJson.h>
#include <DNSServer.h>

extern WebServer server;

void webServer_Task(void* pvParameters);
void handle_OnConnect();
void handle_NotFound();
#endif
