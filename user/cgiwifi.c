#include <esp8266.h>
#include "httpd.h"
#include "rboot.h"

// This callback allows us to restart the system after we've responded nicely to the request
static void ICACHE_FLASH_ATTR resetTimerCb(void *arg) {
  system_restart();
}

int ICACHE_FLASH_ATTR cgiWifiSettings(HttpdConnData *connData) {
  if (connData->conn==NULL) return HTTPD_CGI_DONE;
  int len;
  bool restart = false;
  bool connect = false;
  char buff[128];
  static ETSTimer postTimer;
  //static struct ip_info ipInfo;
  static struct station_config stconf;
  static struct softap_config  apconf;

  // get the current configuration
  wifi_station_get_config(&stconf);
  wifi_softap_get_config(&apconf);

  // Configure the WiFi mode if it has been set
  len=httpdFindArg(connData->post->buff, "wifiMode", buff, sizeof(buff));
  os_printf("len: %d\n", len);
  if (len>0) {
    os_printf("cgiWifiSetMode: %s\n", buff);
    uint8 mode = atoi(buff);
    if(mode != wifi_get_opmode()){
      wifi_set_opmode(mode);
      restart = true;
    }
  }

  // Configure the WiFi client settings
  // Configure the SSID of the access point to connect to
  len = httpdFindArg(connData->post->buff, "clientSSID", buff, sizeof(buff));
  if(len>0){
    if(os_strcmp((const char *)stconf.ssid, buff) != 0){
      // the value has changed
      os_printf("Setting Client SSID: %s\n", buff);
      os_strncpy((char*)stconf.ssid, buff, 32);
      connect = true;
    }
  }

  // Configure the password to use to connect to the access point
  len = httpdFindArg(connData->post->buff, "clientPasswd", buff, sizeof(buff));
  if(len>0){
    if(os_strcmp((const char *)stconf.password, buff) != 0){
      // the value has changed
      os_printf("Setting Client Passwd: %s\n", buff);
      os_strncpy((char*)stconf.password, buff, 64);
      connect = true;
    }
  }

  // Configure the WiFi AP settings
  //APAuth
  len = httpdFindArg(connData->post->buff, "APAuth", buff, sizeof(buff));
  if(len>0){
    os_printf("Setting APAuth: %d\n", atoi(buff));
    apconf.authmode = atoi(buff);
    restart = true;
  }
  //APChannel
  len = httpdFindArg(connData->post->buff, "APChannel", buff, sizeof(buff));
  if(len>0){
    os_printf("Setting Channel: %d\n", atoi(buff));
    apconf.channel = atoi(buff);
    restart = true;
  }
  //APSSID
  len = httpdFindArg(connData->post->buff, "APSSID", buff, sizeof(buff));
  if(len>0){
    os_printf("Setting AP SSID: %s\n", buff);
    os_strncpy((char*)apconf.ssid, buff, 32);
    restart = true;
  }
  wifi_softap_set_config(&apconf);

  if(restart){
    //Schedule disconnect/connect
    os_timer_disarm(&postTimer);
    os_timer_setfn(&postTimer, resetTimerCb, NULL);
    os_timer_arm(&postTimer, 500, 0);
  }else  if(connect){
    os_printf("Try to connect to AP %s pw %s\n", stconf.ssid, stconf.password);
    if (wifi_get_opmode() != STATIONAP_MODE) wifi_set_opmode(STATIONAP_MODE);
    wifi_station_disconnect();
    ETS_UART_INTR_DISABLE();
    wifi_station_set_config(&stconf);
    ETS_UART_INTR_ENABLE();
    wifi_station_connect();
  }

  httpdSend(connData, "HTTP/1.0 204 No Content\r\nServer: esp8266-httpd/0.1\r\n\r\n", -1);
  return HTTPD_CGI_DONE;
}

int ICACHE_FLASH_ATTR tplWlanInfo(HttpdConnData *connData, char *token, void **arg) {
  char buff[1024];
  static struct station_config stconf;
  struct ip_info ipconfig;
  struct softap_config ap_conf;

  if (token==NULL) return HTTPD_CGI_DONE;
  wifi_station_get_config(&stconf);

  buff[0] = 0;
  if (os_strcmp(token, "wifiMode")==0) {
    os_sprintf(buff, "%d", wifi_get_opmode());
  } else if (os_strcmp(token, "clientSSID")==0) {
    os_strcpy(buff, (char*)stconf.ssid);
  } else if (os_strcmp(token, "clientPasswd")==0) {
    os_strcpy(buff, (char*)stconf.password);
  } else if (os_strcmp(token, "clientIp")==0) {
    if(wifi_get_opmode() == SOFTAP_MODE) return HTTPD_CGI_DONE;
    wifi_get_ip_info(STATION_IF, &ipconfig);
    os_sprintf(buff, IPSTR, IP2STR(&ipconfig.ip));
  } else if (os_strcmp(token, "clientGateway")==0) {
    if(wifi_get_opmode() == SOFTAP_MODE) return HTTPD_CGI_DONE;
    wifi_get_ip_info(STATION_IF, &ipconfig);
    os_sprintf(buff, IPSTR, IP2STR(&ipconfig.gw));
  } else if (os_strcmp(token, "clientNetmask")==0) {
    if(wifi_get_opmode() == SOFTAP_MODE) return HTTPD_CGI_DONE;
    wifi_get_ip_info(STATION_IF, &ipconfig);
    os_sprintf(buff, IPSTR, IP2STR(&ipconfig.netmask));
  } else if (os_strcmp(token, "clientDhcp")==0) {
    os_sprintf(buff, (wifi_station_dhcpc_status() == DHCP_STARTED ? "true" : "false"));
  } else if (os_strcmp(token, "clientMac")==0) {
    uint8 mac[6];
    wifi_get_macaddr(STATION_IF, mac);
    os_sprintf(buff, MACSTR, MAC2STR(mac));
  } else if (os_strcmp(token, "clientConnected")==0) {
    os_sprintf(buff, "%d", wifi_station_get_connect_status());
  } else if (os_strcmp(token, "clientSignal")==0) {
    os_sprintf(buff, "%d", wifi_station_get_rssi());
  } else if (os_strcmp(token, "APAuth")==0) {
    wifi_softap_get_config(&ap_conf);
    os_sprintf(buff, "%d", ap_conf.authmode);
  } else if (os_strcmp(token, "APChannel")==0) {
    wifi_softap_get_config(&ap_conf);
    os_sprintf(buff, "%d", ap_conf.channel);
  } else if (os_strcmp(token, "APSSID")==0) {
    wifi_softap_get_config(&ap_conf);
    os_sprintf(buff, "%s", ap_conf.ssid);
  } else if (os_strcmp(token, "version")==0) {
    os_sprintf(buff, "%s", VERSION);
  } else if (os_strcmp(token, "romSlot")==0) {
    os_sprintf(buff, "%d", rboot_get_current_rom());
  }

  httpdSend(connData, buff, -1);
  return HTTPD_CGI_DONE;
}

//Cgi that allows the WiFi firmware to be updated via http
int ICACHE_FLASH_ATTR cgiWifiReset(HttpdConnData *connData) {
  static struct station_config stconf;

  if(connData->requestType == HTTPD_METHOD_POST){
    wifi_station_get_config(&stconf);
    memset(stconf.ssid,0,sizeof(stconf.ssid));
    memset(stconf.password,0,sizeof(stconf.password));
    wifi_station_disconnect();
    wifi_station_set_config(&stconf);
    httpdSend(connData, "HTTP/1.0 204 No Content\r\nServer: esp8266-httpd/0.3\r\n\r\n", -1);
  }else{
    httpdSend(connData, "HTTP/1.0 200 OK\r\nServer: esp8266-httpd/0.3\r\nContent-Length: 105\r\n\r\n<html><body><form action=\"\" method=\"post\"><input type=\"submit\" value=\"Reset WiFi Settings\"></form></body>", -1);
  }
  return HTTPD_CGI_DONE;
}