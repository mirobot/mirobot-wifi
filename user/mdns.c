#include <esp8266.h>
#include "mdns.h"

static ETSTimer mdnsTimer;

static void ICACHE_FLASH_ATTR mdnsTimerCb(void *arg) {
  os_timer_disarm(&mdnsTimer);
  mdnsInit();
}

void ICACHE_FLASH_ATTR mdnsInit()
{
  struct ip_info sta_ip;
  wifi_get_ip_info(STATION_IF, &sta_ip);
  
  if(sta_ip.ip.addr == 0){
    os_printf("\nDelaying mDNS\n");
    os_timer_disarm(&mdnsTimer);
    os_timer_setfn(&mdnsTimer, mdnsTimerCb, NULL);
    os_timer_arm(&mdnsTimer, 10000, 0);
    return;
  }
  
  os_printf("\nStarting mDNS\n");
  struct mdns_info *info = (struct mdns_info *)os_zalloc(sizeof(struct mdns_info));
  info->host_name = "Mirobot";
  info->ipAddr = sta_ip.ip.addr; //ESP8266 station IP
  info->server_name = "http";
  info->server_port = 80;
  info->txt_data[0] = "version = "VERSION;
  wifi_set_broadcast_if(STATIONAP_MODE);
  espconn_mdns_init(info);
}