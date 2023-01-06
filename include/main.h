#ifndef MAIN_H
#define MAIN_H

//是否启用DEBUG调试输出
#define DEBUG
#ifdef DEBUG
#define CONFIG_ARDUHAL_LOG_COLORS 1
#define FTP_SERVER_DEBUG
#include <esp_log.h>
#endif

#include <esp_task_wdt.h>
#include <Wire.h>
#include "FS.h"
#include "FFat.h"
#include "ArduinoJson.h"
#include <WiFi.h>
#define FILESYSTEM FFat
#if FILESYSTEM == FFat
#include <FFat.h>
#endif
#if FILESYSTEM == SPIFFS
#include <SPIFFS.h>
#endif

#define I2C_SCL 22
#define I2C_SDA 19
#define CONFIG_FILE_PATH "/config.json"
#define DEVICE_NAME "ESP32 血氧仪"
#define EVENT_NETWORK_OK B1
extern EventGroupHandle_t Event_Handler;
extern DynamicJsonDocument config_json;

void loadDefaultConfig();
uint8_t saveConfigFile();
#endif
