#include <osapi.h>
#include "user_interface.h"
#include "mem.h"
#include "gpio.h"
#include "espmissingincludes.h"
#include "driver/uart.h"
#include "arduino.h"

static uint8 rcvBuffer[ARDUINO_PAGE_SIZE + 20];
static uint8 rcvBufferCounter = 0;
uint8 pageBuffer[ARDUINO_PAGE_SIZE];

static bool in_progress  = false;
arduinoUpdateState_t arduinoUpdateState;
static ETSTimer commsTimeout;

void arduinoCommsTimeout(){
  arduinoUpdateState.state = IDLE;
  rcvBufferCounter = 0;
}

void setTimeout(){
  //Schedule disconnect/connect
  os_timer_disarm(&commsTimeout);
  os_timer_setfn(&commsTimeout, arduinoCommsTimeout, NULL);
  os_timer_arm(&commsTimeout, 500, 0);
}

void arduinoConnect(){
  if(arduinoUpdateState.state == CONNECT){
    // Send the "connect" command
    // '0 '
    uart0_sendStr("0 ");
  
    //Schedule another attempt if it's not ready yet
    os_timer_disarm(&commsTimeout);
    os_timer_setfn(&commsTimeout, arduinoConnect, NULL);
    os_timer_arm(&commsTimeout, 300, 0);
  }else{
    os_timer_disarm(&commsTimeout);
  }
}

void arduinoLoadAddress(int addr){
  // Send the "load address" command
  // U
  // addr >> 1 & 0xFF
  // (addr >> 9) & 0xFF
  // ' '
  uint8 temp[] = "U\0\0 ";
  temp[1] = (addr >> 1) & 0xFF;
  temp[2] = (addr >> 9) & 0xFF;
  uart0_tx_buffer(temp, 4);
  setTimeout();
}

void arduinoProgramPage(uint8 * data, int len){
  // Send the "program page" command
  // d
  // len & 0xFF
  // (len >> 8) & 0xFF
  // 'F'
  // data
  // ' '
  uint8 temp[] = "d\0\0F";
  temp[1] = (len >> 8) & 0xFF;
  temp[2] = len & 0xFF;
  uart0_tx_buffer(temp, 4);
  uart0_tx_buffer(data, len);
  uart0_sendStr(" ");
  setTimeout();
}

void arduinoReadPage(int len){
  // Send the "read page" command
  // t
  // len & 0xFF
  // (len >> 8) & 0xFF
  // 'F '
  uint8 temp[] = "t\0\0F ";
  temp[1] = (len >> 8) & 0xFF;
  temp[2] = len & 0xFF;
  uart0_tx_buffer(temp, 5);
  setTimeout();
}

void arduinoReset(){
  // Causes a delay to reset the Arduino
  for(int i=0; i<50; i++){
    gpio_output_set(0, 1, 1, 0);
  }
  gpio_output_set(1, 0, 1, 0);
}

void arduinoUpdate(){
  int addr;
  switch (arduinoUpdateState.state){
  case CONNECT:
    //connect to the arduino
    arduinoConnect();
    break;
  case LOAD_PROG_ADDRESS:
    //load address
    addr = arduinoUpdateState.page * ARDUINO_PAGE_SIZE;
    arduinoLoadAddress(addr);
    break;
  case PROGRAM_PAGE:
    //program page
    spi_flash_read(ARDUINO_FLASH_LOCATION + (ARDUINO_PAGE_SIZE * arduinoUpdateState.page), (uint32 *) &pageBuffer, ARDUINO_PAGE_SIZE);
    arduinoProgramPage(pageBuffer, ARDUINO_PAGE_SIZE);
    break;
  case LOAD_READ_ADDRESS:
    //load address
    addr = arduinoUpdateState.page * ARDUINO_PAGE_SIZE;
    arduinoLoadAddress(addr);
    break;
  case READ_PAGE:
    //read page
    arduinoReadPage(ARDUINO_PAGE_SIZE);
    break;
  case IDLE:
    return;
  }
}

bool arduinoBeginUpdate(){
  if(arduinoUpdateState.state == IDLE){
    arduinoUpdateState.page = 0;
    arduinoUpdateState.state = CONNECT;
    in_progress = true;
    //toggle the reset pin
    arduinoReset();
    //start the update process
    arduinoUpdate();
    return true;
  }else{
    return false;
  }
}

char arduinoGetStatus(){
  // Return how many pages have been sent so far to track progress
  return 0;
}

bool arduinoUpdating(){
  return arduinoUpdateState.state != IDLE;
}

bool okMsg(){
  return (rcvBufferCounter >= 2 && rcvBuffer[0] == 20 && rcvBuffer[1] == 16);
}

void arduinoHandleData(uint8 incoming){
  rcvBuffer[rcvBufferCounter++] = incoming;
  // check if we have an appropriate response
  switch (arduinoUpdateState.state){
  case CONNECT:
    if(okMsg()){
      arduinoUpdateState.state = LOAD_PROG_ADDRESS;
      rcvBufferCounter = 0;
      arduinoUpdate();
    }
    break;
  case LOAD_PROG_ADDRESS:
    if(okMsg()){
      arduinoUpdateState.state = PROGRAM_PAGE;
      rcvBufferCounter = 0;
      arduinoUpdate();
    }
    break;
  case PROGRAM_PAGE:
    if(okMsg()){
      arduinoUpdateState.state = LOAD_READ_ADDRESS;
      rcvBufferCounter = 0;
      arduinoUpdate();
    }
    break;
  case LOAD_READ_ADDRESS:
    if(okMsg()){
      arduinoUpdateState.state = READ_PAGE;
      rcvBufferCounter = 0;
      arduinoUpdate();
    }
    break;
  case READ_PAGE:
    if (rcvBufferCounter >= (2 + ARDUINO_PAGE_SIZE) && rcvBuffer[0] == 20 && rcvBuffer[1 + ARDUINO_PAGE_SIZE] == 16) {
      //it's a full packet with a page worth of data, let's check it matches what's in the buffer we sent
      if(memcmp(&rcvBuffer[1], &pageBuffer, ARDUINO_PAGE_SIZE) == 0){
        // it is correct
        if(++arduinoUpdateState.page >= ARDUINO_MAX_PAGE){
          os_timer_disarm(&commsTimeout);
          arduinoUpdateState.state = IDLE;
        }else{
          arduinoUpdateState.state = LOAD_PROG_ADDRESS;
          rcvBufferCounter = 0;
          arduinoUpdate();
        }
      }else{
        os_timer_disarm(&commsTimeout);
        arduinoUpdateState.state = IDLE;
      }
    }
    break;
  case IDLE:
    return;
  }
}
