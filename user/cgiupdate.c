#include <esp8266.h>
#include "cgiflash.h"
#include "arduino.h"
#include "rboot.h"


//Cgi that reads the SPI flash. Assumes 512KByte flash.
int ICACHE_FLASH_ATTR cgiReadFlashChunk(HttpdConnData *connData) {
  int *pos=(int *)&connData->cgiData;
  char temp[10];
  int offset = 0;
  int length = 512*1024;

  if (connData->conn==NULL) {
    //Connection aborted. Clean up.
    return HTTPD_CGI_DONE;
  }

  if(httpdFindArg(connData->getArgs, "offset", temp, sizeof(temp)) > 0){
    offset = atoi(temp);
  }
  if(httpdFindArg(connData->getArgs, "length", temp, sizeof(temp)) > 0){
    length = atoi(temp);
  }
  os_printf("Offset: %d Length: %d\n", offset, length);

  if (*pos==0) {
    os_printf("Start flash download.\n");
    httpdStartResponse(connData, 200);
    httpdHeader(connData, "Content-Type", "application/octet-stream; charset=utf-8");
    httpdEndHeaders(connData);
    *pos=0x40200000 + offset;
    return HTTPD_CGI_MORE;
  }
  //Send 1K of flash per call. We will get called again if we haven't sent 512K yet.
  int remaining = 0x40200000+offset+length - *pos;
  espconn_sent(connData->conn, (uint8 *)(*pos), remaining > 1024 ? 1024 : remaining);
  *pos+=1024;
  if (*pos>=0x40200000+offset+length) return HTTPD_CGI_DONE; else return HTTPD_CGI_MORE;
}

int ICACHE_FLASH_ATTR write_post_to_flash(HttpdConnData *connData, int max_len, int base_loc){
  if (connData->conn==NULL) {
    //Connection aborted. Clean up.
    return HTTPD_CGI_DONE;
  }
  if(connData->post->len > max_len){
    // The uploaded file is too large
    os_printf("File too large\n");
    httpdSend(connData, "HTTP/1.0 500 Internal Server Error\r\nServer: esp8266-httpd/0.3\r\nContent-Type: text/plain\r\nContent-Length: 17\r\n\r\nFile too large.\r\n", -1);
    return HTTPD_CGI_DONE;
  }

  // The source should be 4byte aligned, so go ahead and flash whatever we have
  int address = base_loc + connData->post->received - connData->post->buffLen;
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

//Cgi that allows the Arduino firmware to be updated via http
int ICACHE_FLASH_ATTR cgiUploadArduino(HttpdConnData *connData) {
  char temp[10];
  bool flash = false;

  if(connData->requestType == HTTPD_METHOD_POST){
    if(httpdFindArg(connData->getArgs, "flash", temp, sizeof(temp)) > 0){
      flash = !strcmp(temp, "true");
    }

    if(flash){
      os_printf("Flashing Arduino\n");
      if(arduinoBeginUpdate()){
        httpdSend(connData, "HTTP/1.0 204 No Content\r\nServer: esp8266-httpd/0.3\r\n\r\n", -1);
      }else{
        httpdSend(connData, "HTTP/1.0 500 Server Error\r\nServer: esp8266-httpd/0.3\r\n\r\n", -1);
      }
    }else{
      return write_post_to_flash(connData, ARDUINO_MAX_SIZE, ARDUINO_FLASH_LOCATION);
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

//Cgi that allows the Arduino firmware to be updated via http
int ICACHE_FLASH_ATTR cgiUploadEspfs(HttpdConnData *connData) {
  if(connData->requestType == HTTPD_METHOD_POST){
    return write_post_to_flash(connData, ESPFS_SIZE, ESPFS_POS);
  }
  return HTTPD_CGI_DONE;
}

static void ICACHE_FLASH_ATTR resetTimerCb(void *arg) {
  system_restart();
}

//Cgi that allows the WiFi firmware to be updated via http
int ICACHE_FLASH_ATTR cgiUploadWifi(HttpdConnData *connData) {
  static ETSTimer postTimer;
  char temp[10];
  bool commit = false;
  int loc;

  if(connData->requestType == HTTPD_METHOD_POST){
    if(httpdFindArg(connData->getArgs, "commit", temp, sizeof(temp)) > 0){
      commit = !strcmp(temp, "true");
    }

    if(commit){
      // Switch to the other boot rom
      if(rboot_switch_rom()){
        os_timer_disarm(&postTimer);
        os_timer_setfn(&postTimer, resetTimerCb, NULL);
        os_timer_arm(&postTimer, 500, 0);
        httpdSend(connData, "HTTP/1.0 204 No Content\r\nServer: esp8266-httpd/0.3\r\n\r\n", -1);
      }else{
        httpdSend(connData, "HTTP/1.0 500 Server Error\r\nServer: esp8266-httpd/0.3\r\n\r\n", -1);
      }
    }else{
      if(rboot_get_current_rom() == 0){
        // Write to rom location 1
        loc = 0x39000;
      }else if(rboot_get_current_rom() == 1){
        // Write to rom location 0
        loc = 0x02000;
      }else{
        // There's an error, exit;
        return HTTPD_CGI_DONE;
      }
      return write_post_to_flash(connData, (220 * 1024), loc);
    }
  }
  return HTTPD_CGI_DONE;
}