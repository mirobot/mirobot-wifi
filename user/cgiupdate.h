#ifndef CGIARDUINO_H
#define CGIARDUINO_H

#include "httpd.h"

int cgiUploadArduino(HttpdConnData *connData);
int cgiUploadWifi(HttpdConnData *connData);
int cgiReadFlashChunk(HttpdConnData *connData);

#endif