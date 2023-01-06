#ifndef FTP_H
#define FTP_H
#include <SimpleFTPServer.h>
extern FtpServer ftpSrv;
void FTP_task(void* pvParameters);
#endif
