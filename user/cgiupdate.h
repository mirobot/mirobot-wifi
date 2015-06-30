#ifndef CGIARDUINO_H
#define CGIARDUINO_H

#include "httpd.h"

int cgiUploadArduino(HttpdConnData *connData);
int cgiReadFlashChunk(HttpdConnData *connData);

#endif