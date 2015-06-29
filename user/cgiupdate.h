#ifndef CGIARDUINO_H
#define CGIARDUINO_H

#include "httpd.h"

int cgiArduinoUpload(HttpdConnData *connData);
int cgiArduinoFlash(HttpdConnData *connData);

#endif