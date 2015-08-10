#ifndef CGIARDUINO_H
#define CGIARDUINO_H

#include "httpd.h"

int cgiUploadEspfs(HttpdConnData *connData);
int cgiUploadArduino(HttpdConnData *connData);
int cgiUploadWifi(HttpdConnData *connData);
int cgiReadFlashChunk(HttpdConnData *connData);

#endif