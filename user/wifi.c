#include <esp8266.h>
#include "wifi.h"
#include "mdns.h"

static char ssid[33];
static uint8_t macaddr[6];
static struct ip_info ipconfig;
static ETSTimer wifiTimer;

static void ICACHE_FLASH_ATTR wifiTimerCb(void *arg) {
  os_printf("\nChecking WiFi\n");
  if(ipconfig.ip.addr == 0){
    os_printf("\nWiFi not connected to STA - disabling\n");
    wifi_set_opmode(SOFTAP_MODE);
  }else{
    os_printf("\nWiFi connected to STA\n");
  }
}

void wifi_handle_event_cb(System_Event_t *evt)
{
  switch (evt->event) {
    case EVENT_STAMODE_CONNECTED:
      break;
    case EVENT_STAMODE_DISCONNECTED:
      break;
    case EVENT_STAMODE_AUTHMODE_CHANGE:
      break;
    case EVENT_STAMODE_GOT_IP:
      os_printf("Connected to WIFi station\n");
      os_timer_disarm(&wifiTimer);
      mdnsInit();
      break;
    case EVENT_SOFTAPMODE_STACONNECTED:
      break;
    case EVENT_SOFTAPMODE_STADISCONNECTED:
      break;
    default:
      break;
  }
}

void ICACHE_FLASH_ATTR wifiInit()
{
  struct softap_config apConfig;
  struct ip_info info;

  wifi_set_event_handler_cb(wifi_handle_event_cb);

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
  apConfig.beacon_interval = 100;
  wifi_softap_set_config(&apConfig);

  os_timer_disarm(&wifiTimer);
  os_timer_setfn(&wifiTimer, wifiTimerCb, NULL);
  os_timer_arm(&wifiTimer, 10000, 0);
}