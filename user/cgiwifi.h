#ifndef CGIWIFI_H
#define CGIWIFI_H

#include "httpd.h"

int cgiWiFiScan(HttpdConnData *connData);
int cgiWifiSettings(HttpdConnData *connData);
int tplWlanInfo(HttpdConnData *connData, char *token, void **arg);
int cgiWifiReset(HttpdConnData *connData);

#endif