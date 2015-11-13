#include <esp8266.h>
#include "wifi.h"

static char ssid[33];
//static char password[33];
static uint8_t macaddr[6];

void ICACHE_FLASH_ATTR wifiInit()
{
  struct softap_config apConfig;
  struct ip_info info;

  if (wifi_get_opmode() != STATIONAP_MODE) wifi_set_opmode(STATIONAP_MODE);

  IP4_ADDR(&info.ip, 10, 10, 100, 254);
  IP4_ADDR(&info.gw, 10, 10, 100, 1);
  IP4_ADDR(&info.netmask, 255, 255, 255, 0);
  wifi_set_ip_info(SOFTAP_IF, &info);

  wifi_get_macaddr(SOFTAP_IF, macaddr);
  wifi_softap_get_config(&apConfig);
  os_memset(apConfig.password, 0, sizeof(apConfig.password));

  os_memset(apConfig.ssid, 0, sizeof(apConfig.ssid));
  os_sprintf(ssid, "%s%02X%02X", WIFI_APSSID_BASE, macaddr[4], macaddr[5]);
  os_memcpy(apConfig.ssid, ssid, strlen(ssid));

  apConfig.ssid_len = strlen(ssid);
  apConfig.authmode = AUTH_OPEN;
  apConfig.channel = 7;
  apConfig.max_connection = 3;
  apConfig.ssid_hidden = 0;
  wifi_softap_set_config(&apConfig);
}