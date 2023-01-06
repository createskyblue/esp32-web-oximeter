#include "main.h"
#include "FTP.h"
FtpServer ftpSrv;   //set #define FTP_DEBUG in ESP8266FtpServer.h to see ftp verbose on serial

void _callback(FtpOperation ftpOperation, unsigned int freeSpace, unsigned int totalSpace){
  switch (ftpOperation) {
    case FTP_CONNECT:
      Serial.println(F("FTP: Connected!"));
      break;
    case FTP_DISCONNECT:
      Serial.println(F("FTP: Disconnected!"));
      break;
    case FTP_FREE_SPACE_CHANGE:
      Serial.printf("FTP: Free space change, free %u of %u!\n", freeSpace, totalSpace);
      break;
    default:
      break;
  }
};
void _transferCallback(FtpTransferOperation ftpOperation, const char* name, unsigned int transferredSize){
  switch (ftpOperation) {
    case FTP_UPLOAD_START:
      Serial.println(F("FTP: Upload start!"));
      break;
    case FTP_UPLOAD:
      Serial.printf("FTP: Upload of file %s byte %u\n", name, transferredSize);
      break;
    case FTP_TRANSFER_STOP:
      Serial.println(F("FTP: Finish transfer!"));
      break;
    case FTP_TRANSFER_ERROR:
      Serial.println(F("FTP: Transfer error!"));
      break;
    default:
      break;
  }

  /* FTP_UPLOAD_START = 0,
   * FTP_UPLOAD = 1,
   *
   * FTP_DOWNLOAD_START = 2,
   * FTP_DOWNLOAD = 3,
   *
   * FTP_TRANSFER_STOP = 4,
   * FTP_DOWNLOAD_STOP = 4,
   * FTP_UPLOAD_STOP = 4,
   *
   * FTP_TRANSFER_ERROR = 5,
   * FTP_DOWNLOAD_ERROR = 5,
   * FTP_UPLOAD_ERROR = 5
   */
};

void FTP_task(void* pvParameters) {
    //需要先等待网络启动
    xEventGroupWaitBits(Event_Handler,      //事件的句柄
                            EVENT_NETWORK_OK,    //感兴趣的事件
                            pdFALSE,             //退出时是否清除事件位
                            pdTRUE,             //是否满足所有事件
                            portMAX_DELAY);     //超时时间，一直等所有事件都满足

    // ftpSrv.setCallback(_callback);
    // ftpSrv.setTransferCallback(_transferCallback);

    Serial.println("Start FTP with user: user and passwd: password!");
    ftpSrv.begin();    //username, password for ftp.   (default 21, 50009 for PASV)
    while (1) {
        ftpSrv.handleFTP();        //make sure in loop you call handleFTP()!!
        vTaskDelay(5);
    }
}