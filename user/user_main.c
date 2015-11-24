#include <esp8266.h>
#include "httpd.h"
#include "websocket.h"
#include "gpio.h"
#include "httpdespfs.h"
#include "cgiwifi.h"
#include "cgiflash.h"
#include "cgiupdate.h"
#include "driver/uart.h"
#include "arduino.h"
#include "auth.h"
#include "espfs.h"
#include "captdns.h"
#include "webpages-espfs.h"
#include "rboot.h"
#include "wifi.h"
#include "mdns.h"

char inputBuffer[100];
int inputBufferCounter = 0;

// This is the main url->function dispatching data struct.
HttpdBuiltInUrl builtInUrls[]={
  //{"*", cgiRedirectApClientToHostname, "mirobot.local"},
  {"/", cgiRedirect, "/index.html"},
  
  // admin functions
  {"/admin/updateui.cgi", cgiUploadEspfs, NULL},
  {"/admin/updatewifi.cgi", cgiUploadWifi, NULL},
  {"/admin/updatearduino.cgi", cgiUploadArduino, NULL},
  {"/admin/readflash.bin", cgiReadFlashChunk, NULL},
  {"/admin/settings.json", cgiEspFsTemplate, tplWlanInfo},
  {"/admin/update.html", cgiEspFsTemplate, tplWlanInfo},
  {"/admin/wifi.html", cgiEspFsTemplate, tplWlanInfo},
  {"/admin/wifiscan.cgi", cgiWiFiScan, NULL},
  {"/admin/settings.cgi", cgiWifiSettings, NULL},
  
  {"/index.html", cgiEspFsTemplate, tplWlanInfo},

  {"*", cgiEspFsHook, NULL}, //Catch-all cgi function for the filesystem
  {NULL, NULL, NULL}
};

void serialHandler(uint8 incoming){
  if(arduinoUpdating()){
    arduinoHandleData(incoming);
  }else{
    if(incoming == '\r' || incoming == '\n'){
      if(inputBufferCounter > 0){
        // Send the string
        inputBuffer[inputBufferCounter] = 0;
        wsSend((char *)inputBuffer);
        inputBufferCounter = 0;
      }
    }else{
      inputBuffer[inputBufferCounter++] = incoming;
    }
  }
}

void wsHandler(int conn_no, char *msg){
  // Send the received WebSocket payload out via the serial port
  //os_printf("RECV: %s\n", msg);
  uart0_sendStr(msg);
  uart0_sendStr("\r\n");
}

void ioInit(){
  gpio_output_set(1, 0, 1, 0);
  // disable pullup and pulldown on GPIO5 and configure as output
  PIN_PULLUP_DIS(PERIPHS_IO_MUX_GPIO5_U);
  PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO5_U, FUNC_GPIO5);
  PIN_PULLUP_DIS(PERIPHS_IO_MUX_GPIO0_U);
  PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0);
  // reset Arduino
  arduinoReset();
}

//#define SHOW_HEAP_USE
#ifdef SHOW_HEAP_USE
static ETSTimer prHeapTimer;

static void ICACHE_FLASH_ATTR prHeapTimerCb(void *arg) {
	os_printf("Heap: %ld\n", (unsigned long)system_get_free_heap_size());
}
#endif

//Main routine. Initialize stdout, the I/O, filesystem and the webserver and we're done.
void user_init(void) {
  wifiInit();
  uart_init(BIT_RATE_57600, BIT_RATE_115200);
  espFsInit((void*)(0x40200000 + ESPFS_POS));
  install_uart0_rx_handler(serialHandler);
  ioInit();
  captdnsInit();
  httpdInit(builtInUrls, 80);
  wsInit(8899, wsHandler);
  mdnsInit();
#ifdef SHOW_HEAP_USE
	os_timer_disarm(&prHeapTimer);
	os_timer_setfn(&prHeapTimer, prHeapTimerCb, NULL);
	os_timer_arm(&prHeapTimer, 3000, 1);
#endif
  os_printf("\nReady\n");
}
