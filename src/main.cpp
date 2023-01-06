#include "main.h"
#include "cal_BPM_SpO2.h"
#include "web.h"
#include "FTP.h"
MAX30105 particleSensor;
DNSServer dnsServer;
//配置json
DynamicJsonDocument config_json(1024);
char configFile_json[1024];
//网络启动事件
EventGroupHandle_t Event_Handler = NULL;

IPAddress AP_local_ip(192, 168, 4, 1);
IPAddress AP_gateway(192, 168, 4, 1); 
IPAddress AP_subnet(255, 255, 255, 0);

void WiFiEvent(WiFiEvent_t event);

//恢复默认配置
void loadDefaultConfig() {
    log_d("loadDefaultConfig");
    config_json["AP_ssid"]    = "ESP32 血氧仪";
    config_json["AP_passwd"]  = "";
    config_json["STA_ssid"]   = "";
    config_json["STA_passwd"] = "";
}
//载入配置文件
void loadConfigFile() {
    log_d("loadConfigFile");
    //读取配置文件
    File configFile_f = FILESYSTEM.open(CONFIG_FILE_PATH);
    if(!configFile_f || configFile_f.isDirectory()){
        log_w("打开配置文件%s失败,使用默认配置",CONFIG_FILE_PATH);
        return;
    }
    //读取配置文件
    configFile_f.read((uint8_t*)configFile_json, sizeof(configFile_json));
    configFile_f.close();
    log_d("%s",configFile_json);
    //反序列化json
    DeserializationError error = deserializeJson(config_json, configFile_json);
    if (error) {
        log_w("反序列化配置文件失败:%s,使用默认配置",error.f_str());
        loadDefaultConfig();
        return;
    }
    serializeJsonPretty(config_json, Serial);
}
uint8_t saveConfigFile() {
    char configFileSave_json[1024];
    //序列化配置文件
    serializeJson(config_json, configFileSave_json, sizeof(configFileSave_json));
    log_d("序列化后配置文件输出:%s",configFileSave_json);
    //打开配置文件
    File file = FILESYSTEM.open(CONFIG_FILE_PATH, FILE_WRITE);
    if(!file){
        log_e("failed to open file for writing");
        return 1;
    }
    //写入配置文件
    if(!file.print(configFileSave_json)){
        file.close();
        log_e("write failed");
        return 1;
    }
    file.close();
    return 0;
}


void setup()
{
    //初始化串口
    Serial.begin(115200);
    //彩色log打印测试
    // log_e("E");
    // log_w("W");
    // log_i("I");
    // log_d("D");
    // log_v("V");

    //创建事件
    Event_Handler = xEventGroupCreate();
    if (Event_Handler == NULL) {
        log_e("事件创建失败");
        abort();
    }

    //初始化文件系统
    log_i("Inizializing FS...");
    if (!FILESYSTEM.begin(true)) {
        log_e("文件系统挂载失败，请检查分区表设置！");
        abort();
    }

    //载入默认配置
    loadDefaultConfig();
    //载入配置文件
    loadConfigFile();

    WiFi.softAPConfig(AP_local_ip, AP_gateway, AP_subnet);
    //启动Wifi连接
    WiFi.mode(WIFI_AP_STA);
    //注册Wifi回调事件
    WiFi.onEvent(WiFiEvent);
    serializeJsonPretty(config_json, Serial);
    WiFi.softAP((const char*)config_json["AP_ssid"], (const char*)config_json["AP_passwd"]);
    WiFi.begin((const char*)config_json["STA_ssid"], (const char*)config_json["STA_passwd"]);

    //开启dns服务，用于强制门户验证跳转到主页
    dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
    dnsServer.start(53, "*", AP_gateway);

    //触发 启动完成 事件
    xEventGroupSetBits(Event_Handler, EVENT_NETWORK_OK);
    
    //开启 心率&血压 监测任务
    if (xTaskCreate(
        cal_BPM_SpO2_task,
        "cal_BPM_SpO2",
        4096, /* Stack depth - small microcontrollers will use much
        less stack than this. */
        NULL, /* This example does not use the task parameter. */
        1, /* This task will run at priority 1. */
        NULL ) /* This example does not use the task handle. */ != pdPASS) {
            log_e("Couldn't create cal_BPM_SpO2 task\n"); 
    }

    //开启网络服务
    if (xTaskCreate(
        webServer_Task,
        "webServer",
        8096, /* Stack depth - small microcontrollers will use much
        less stack than this. */
        NULL, /* This example does not use the task parameter. */
        1, /* This task will run at priority 1. */
        NULL ) /* This example does not use the task handle. */ != pdPASS) {
            log_e("Couldn't create cal_BPM_SpO2 task\n"); 
    }

    //开启FTP服务
    if (xTaskCreate(
        FTP_task,
        "FTP",
        8096, /* Stack depth - small microcontrollers will use much
        less stack than this. */
        NULL, /* This example does not use the task parameter. */
        1, /* This task will run at priority 1. */
        NULL ) /* This example does not use the task handle. */ != pdPASS) {
            log_e("Couldn't create FTP task\n"); 
    }
}

void loop()
{
    dnsServer.processNextRequest();
    // vTaskDelay(portMAX_DELAY); 
}

void WiFiEvent(WiFiEvent_t event)
{
    log_d("[WiFi-event] event: %d\n", event);
    switch (event) {
    case SYSTEM_EVENT_WIFI_READY:
        log_d("WiFi interface ready");
        break;
    case SYSTEM_EVENT_SCAN_DONE:
        log_d("Completed scan for access points");
        break;
    case SYSTEM_EVENT_STA_START:
        log_d("WiFi client started");
        break;
    case SYSTEM_EVENT_STA_STOP:
        log_d("WiFi clients stopped");
        break;
    case SYSTEM_EVENT_STA_CONNECTED:
        log_d("Connected to access point");
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        log_d("Disconnected from WiFi access point");
        //尝试重新建立连接
        WiFi.reconnect();
        break;
    case SYSTEM_EVENT_STA_AUTHMODE_CHANGE:
        log_d("Authentication mode of access point has changed");
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        Serial.print("Obtained IP address: ");
        log_d("%s",WiFi.localIP());
        break;
    case SYSTEM_EVENT_STA_LOST_IP:
        log_d("Lost IP address and IP address is reset to 0");
        break;
    case SYSTEM_EVENT_STA_WPS_ER_SUCCESS:
        log_d("WiFi Protected Setup (WPS): succeeded in enrollee mode");
        break;
    case SYSTEM_EVENT_STA_WPS_ER_FAILED:
        log_d("WiFi Protected Setup (WPS): failed in enrollee mode");
        break;
    case SYSTEM_EVENT_STA_WPS_ER_TIMEOUT:
        log_d("WiFi Protected Setup (WPS): timeout in enrollee mode");
        break;
    case SYSTEM_EVENT_STA_WPS_ER_PIN:
        log_d("WiFi Protected Setup (WPS): pin code in enrollee mode");
        break;
    case SYSTEM_EVENT_AP_START:
        log_d("WiFi access point started");
        break;
    case SYSTEM_EVENT_AP_STOP:
        log_d("WiFi access point  stopped");
        break;
    case SYSTEM_EVENT_AP_STACONNECTED:
        log_d("Client connected");
        break;
    case SYSTEM_EVENT_AP_STADISCONNECTED:
        log_d("Client disconnected");
        break;
    case SYSTEM_EVENT_AP_STAIPASSIGNED:
        log_d("Assigned IP address to client");
        break;
    case SYSTEM_EVENT_AP_PROBEREQRECVED:
        log_d("Received probe request");
        break;
    case SYSTEM_EVENT_GOT_IP6:
        log_d("IPv6 is preferred");
        break;
    case SYSTEM_EVENT_ETH_START:
        log_d("Ethernet started");
        break;
    case SYSTEM_EVENT_ETH_STOP:
        log_d("Ethernet stopped");
        break;
    case SYSTEM_EVENT_ETH_CONNECTED:
        log_d("Ethernet connected");
        break;
    case SYSTEM_EVENT_ETH_DISCONNECTED:
        log_d("Ethernet disconnected");
        break;
    case SYSTEM_EVENT_ETH_GOT_IP:
        log_d("Obtained IP address");
        break;
    default: break;
    }
}