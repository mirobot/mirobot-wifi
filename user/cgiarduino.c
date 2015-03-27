#include <string.h>
#include <osapi.h>
#include "user_interface.h"
#include "mem.h"
#include "httpd.h"
#include "arduino.h"
#include <ip_addr.h>
#include "espmissingincludes.h"

//Cgi that allows the ESPFS image to be replaced via http POST
int ICACHE_FLASH_ATTR cgiArduinoUpload(HttpdConnData *connData) {
	if (connData->conn==NULL) {
		//Connection aborted. Clean up.
		return HTTPD_CGI_DONE;
	}
	if(connData->post->len > 30720){
		// The uploaded file is too large
		os_printf("Arduino file too large\n");
		httpdSend(connData, "HTTP/1.0 500 Internal Server Error\r\nServer: esp8266-httpd/0.3\r\nContent-Type: text/plain\r\nContent-Length: 25\r\n\r\nArduino file too large.\r\n", -1);
		return HTTPD_CGI_DONE;
	}
	
	// The source should be 4byte aligned, so go ahead and flash whatever we have
	int address = ARDUINO_FLASH_LOCATION + connData->post->received - connData->post->buffLen;
	if(address % SPI_FLASH_SEC_SIZE == 0){
		// We need to erase this block
		os_printf("Erasing flash at 0x%0X\n", address/SPI_FLASH_SEC_SIZE);
		spi_flash_erase_sector(address/SPI_FLASH_SEC_SIZE);
	}
	// Write the data
	os_printf("Writing at: 0x%0X\n", address);
	spi_flash_write(address, (uint32 *)connData->post->buff, connData->post->buffLen);
	os_printf("Wrote %d bytes (%dB of %d)\n", connData->post->buffLen, connData->post->received, connData->post->len);

	if (connData->post->received == connData->post->len){
		httpdSend(connData, "HTTP/1.0 204 No Content\r\nServer: esp8266-httpd/0.3\r\n\r\n", -1);
		return HTTPD_CGI_DONE;
	} else {
		return HTTPD_CGI_MORE;
	}
}


int ICACHE_FLASH_ATTR cgiArduinoFlash(HttpdConnData *connData) {
  if(connData->requestType == HTTPD_METHOD_POST){
    if(arduinoBeginUpdate()){
      httpdSend(connData, "HTTP/1.0 204 No Content\r\nServer: esp8266-httpd/0.3\r\n\r\n", -1);
    }else{
      httpdSend(connData, "HTTP/1.0 500 Server Error\r\nServer: esp8266-httpd/0.3\r\n\r\n", -1);
    }
  }else if(connData->requestType == HTTPD_METHOD_GET){
    char headbuff[110];
    char buff[40];
    os_sprintf(buff, "{\"state\":\"%s\",\"pages\":%d}", (arduinoUpdating() ? "flashing" : "done"), arduinoPagesFlashed());
    os_sprintf(headbuff, "HTTP/1.0 200 OK\r\nServer: esp8266-httpd/0.3\r\nContent-Type: application/json\r\nContent-Length: %d\r\n\r\n", strlen(buff));
    httpdSend(connData, headbuff, -1);
    httpdSend(connData, buff, -1);
  }
  return HTTPD_CGI_DONE;
}