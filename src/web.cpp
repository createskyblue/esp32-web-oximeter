#include "main.h"
#include "web.h"

WebServer server(80);

String getContentType(String filename) {
    if (server.hasArg("download")) {
        return "application/octet-stream";
    }
    else if (filename.endsWith(".htm")) {
        return "text/html";
    }
    else if (filename.endsWith(".html")) {
        return "text/html";
    }
    else if (filename.endsWith(".css")) {
        return "text/css";
    }
    else if (filename.endsWith(".js")) {
        return "application/javascript";
    }
    else if (filename.endsWith(".png")) {
        return "image/png";
    }
    else if (filename.endsWith(".gif")) {
        return "image/gif";
    }
    else if (filename.endsWith(".jpg")) {
        return "image/jpeg";
    }
    else if (filename.endsWith(".ico")) {
        return "image/x-icon";
    }
    else if (filename.endsWith(".xml")) {
        return "text/xml";
    }
    else if (filename.endsWith(".pdf")) {
        return "application/x-pdf";
    }
    else if (filename.endsWith(".zip")) {
        return "application/x-zip";
    }
    else if (filename.endsWith(".gz")) {
        return "application/x-gzip";
    }
    return "text/plain";
}

bool exists(String path) {
    bool yes = false;
    File file = FILESYSTEM.open(path, "r");
    if (!file.isDirectory() && file) {
        yes = true;
    }
    file.close();
    return yes;
}

bool handleFileRead(String path) {
    log_i("handleFileRead: %s", path);
    if (path.endsWith("/")) {
        path += "index.html";
    }
    String contentType = getContentType(path);
    String pathWithGz = path + ".gz";
    if (exists(pathWithGz) || exists(path)) {
        if (exists(pathWithGz)) {
            path += ".gz";
        }
        File file = FILESYSTEM.open(path, "r");
        server.streamFile(file, contentType);
        file.close();
        return true;
    }
    return false;
}

extern double eSpO2;
extern double Ebpm;
extern float ir_forWeb;
extern float red_forWeb;
extern uint32_t ir, red;
void handle_getBPM_SpO2() {
    char raw_JSON[1024];
    DynamicJsonDocument doc(1024);

    doc["millis"] = millis();
    doc["BPM"] = Ebpm;
    doc["SpO2"] = eSpO2;
    doc["ir_forGraph"] = ir_forWeb;
    doc["red_forGraph"] = red_forWeb;
    doc["ir"] = ir;
    doc["red"] = red;

    serializeJson(doc, raw_JSON);
    server.send(200, "application/json", raw_JSON);
}

extern uint32_t ESP_getFlashChipId();
void handle_getSystemStatus() {
    char compilationDate[50];
    sprintf(compilationDate, "%s %s", __DATE__, __TIME__);
    char raw_JSON[1024];
    DynamicJsonDocument doc(1024);

    doc["millis"] = millis();
    doc["deviceName"] = DEVICE_NAME;
    doc["STA_IP"] = WiFi.localIP();
    doc["compilationDate"] = compilationDate;

    doc["chipModel"] = ESP.getChipModel();
    doc["chipRevision"] = ESP.getChipRevision();
    doc["cpuFreqMHz"] = ESP.getCpuFreqMHz();
    doc["chipCores"] = ESP.getChipCores();

    doc["heapSizeKiB"] = ESP.getHeapSize() / 1024;
    doc["freeHeapKiB"] = ESP.getFreeHeap() / 1024;

    doc["psramSizeKiB"] = ESP.getPsramSize() / 1024;
    doc["freePsramKiB"] = ESP.getFreePsram() / 1024;

    doc["flashChipId"] = ESP_getFlashChipId();
    doc["flashSpeedMHz"] = ESP.getFlashChipSpeed() / 1000000;
    doc["flashSizeMib"] = ESP.getFlashChipSize() / 1024 / 1024;

    doc["sketchMD5"] = ESP.getSketchMD5();
    doc["sdkVersion"] = ESP.getSdkVersion();

    serializeJson(doc, raw_JSON);
    server.send(200, "application/json", raw_JSON);
}

void handle_setAPConfig() {
    DynamicJsonDocument doc(1024);
    //??????????????????
    if (server.args() == 0) {
        return server.send(500, "text/plain", "BAD ARGS");
    }
    //???????????? ?????? json
    DeserializationError error = deserializeJson(doc, server.arg("plain").c_str());
    if (error) {
        log_e("??????????????????????????????",error.f_str());
        server.send(304, "application/json", "{\"msg\":\"write failed\"}");
        return;
    }

    //?????? ????????????
    config_json["AP_ssid"] = doc["AP_ssid"];
    //??????
    if (saveConfigFile()) {
        log_e("????????????????????????");
        server.send(304, "application/json", "{\"msg\":\"write failed\"}");
        return;
    }
    //????????????
    server.send(200, "application/json", "{\"msg\":\"successes\"}");
}
void handle_setSTAConfig() {
    DynamicJsonDocument doc(1024);
    //??????????????????
    if (server.args() == 0) {
        return server.send(500, "text/plain", "BAD ARGS");
    }
    //???????????? ?????? json
    DeserializationError error = deserializeJson(doc, server.arg("plain").c_str());
    if (error) {
        log_e("??????????????????????????????",error.f_str());
        server.send(304, "application/json", "{\"msg\":\"write failed\"}");
        return;
    }

    //?????? ????????????
    config_json["STA_ssid"] = doc["STA_ssid"];
    config_json["STA_passwd"] = doc["STA_passwd"];

    //??????
    if (saveConfigFile()) {
        log_e("????????????????????????");
        server.send(304, "application/json", "{\"msg\":\"write failed\"}");
        return;
    }
    //????????????
    server.send(200, "application/json", "{\"msg\":\"successes\"}");
}
void handle_dirtyHacker() {
    //?????? ???????????????
    server.send(401, "application/json", "{\"msg\":\"Dirty hacker\"}");
}
//??????????????????????????????
void handle_getConfig() {
    String JSON = R""({"AP_ssid":")"";
    JSON += (const char*)config_json["AP_ssid"];
    JSON += R""(","STA_ssid":")"";
    JSON += (const char*)config_json["STA_ssid"];
    JSON += R""("})"";
    server.send(200, "application/json", JSON);
}

void webServer_Task(void* pvParameters) {
    //?????? ????????? ?????????
    server.enableCORS(); //This is the magic
    //??????web????????????
    server.on("/getBPM_SpO2", handle_getBPM_SpO2);
    server.on("/getSystemStatus", handle_getSystemStatus);
    server.on("/setAPConfig", HTTP_POST, handle_setAPConfig);
    server.on("/setSTAConfig", HTTP_POST, handle_setSTAConfig);
    server.on("/getConfig", handle_getConfig);
    //??????????????????????????????
    server.on("/config.json", handle_dirtyHacker);
    server.on("/reboot", []() {
        server.send(200, "application/json", "{\"msg\":\"successes\"}");
        vTaskDelay(1000);
        abort();
    });
    server.onNotFound([]() {
        if (!handleFileRead(server.uri())) {
            server.send(404, "text/plain", "FileNotFound");
        }
        });
    //??????web??????
    server.begin();
    log_d("HTTP server started");

    while (1) {
        esp_task_wdt_reset();
        server.handleClient();
    }
}