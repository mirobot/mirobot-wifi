#include "espmissingincludes.h"
#include "ets_sys.h"
#include "osapi.h"
#include "httpd.h"
#include "websocket.h"
#include "gpio.h"
#include "httpdespfs.h"
#include "cgiwifi.h"
#include "cgiflash.h"
#include "cgiarduino.h"
#include "stdout.h"
#include "driver/uart.h"
#include "arduino.h"

char inputBuffer[100];
int inputBufferCounter = 0;

// This is the main url->function dispatching data struct.
HttpdBuiltInUrl builtInUrls[]={
	{"/", cgiRedirect, "/index.html"},
	
	// admin functions
	{"/updateweb.cgi", cgiUploadEspfs, NULL},
	{"/updatearduino.cgi", cgiArduinoUpload, NULL},
	{"/flasharduino.cgi", cgiArduinoFlash, NULL},
	{"/flash.bin", cgiReadFlash, NULL},

	//Routines to make the /wifi URL and everything beneath it work.
	{"/wifi", cgiRedirect, "/wifi/index.html"},
	{"/wifi/", cgiRedirect, "/wifi/index.html"},
	{"/wifi/wifiscan.cgi", cgiWiFiScan, NULL},
	{"/wifi/index.html", cgiEspFsTemplate, tplWlanInfo},
	{"/wifi/settings.cgi", cgiWifiSettings, NULL},
	{"/wifi/settings.json", cgiEspFsTemplate, tplWlanInfo},

  //Catch-all cgi function for the filesystem
	{"*", cgiEspFsHook, NULL},
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
        wsSend(0, (char *)inputBuffer);
        inputBufferCounter = 0;
      }
    }else{
      inputBuffer[inputBufferCounter++] = incoming;
    }
  }
}

void wsHandler(int conn_no, char *msg){
  // Send the received WebSocket payload out via the serial port
  uart0_sendStr(msg);
  uart0_sendStr("\r\n");
}

void initIO(){
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0);
	gpio_output_set(0, 0, (1<<2), (1<<0));
}

//Main routine. Initialize stdout, the I/O and the webserver and we're done.
void user_init(void) {
	uart_init(BIT_RATE_57600, BIT_RATE_115200);
	install_uart0_rx_handler(serialHandler);
  gpio_output_set(1, 0, 1, 0);
	//stdoutInit();
	initIO();
	httpdInit(builtInUrls, 80);
	wsInit(8899, wsHandler);
	os_printf("\nReady\n");
}
