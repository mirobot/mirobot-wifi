#include <esp8266.h>
#include "mdns.h"

static uint8_t macaddr[6];

void ICACHE_FLASH_ATTR mdnsInit()
{
  char buff[16];
  struct ip_info sta_ip;
  wifi_get_ip_info(STATION_IF, &sta_ip);
  
  if(sta_ip.ip.addr == 0){
    return;
  }
  
  os_printf("\nStarting mDNS\n");
  struct mdns_info *info = (struct mdns_info *)os_zalloc(sizeof(struct mdns_info));
  wifi_get_macaddr(SOFTAP_IF, macaddr);
  os_sprintf(buff, "mirobot-%02x%02x", macaddr[4], macaddr[5]);
  info->host_name = buff;
  info->ipAddr = sta_ip.ip.addr; //ESP8266 station IP
  info->server_name = "http";
  info->server_port = 80;
  info->txt_data[0] = "version = "VERSION;
  wifi_set_broadcast_if(STATIONAP_MODE);
  espconn_mdns_init(info);
}