#ifndef ARDUINO_H
#define ARDUINO_H

#define ARDUINO_PAGE_SIZE 128
#define ARDUINO_MAX_PAGE  240
#define ARDUINO_FLASH_LOCATION 0x76000
#define ARDUINO_MAX_SIZE       30720

typedef enum {
  IDLE,
  CONNECT,
  LOAD_PROG_ADDRESS,
  PROGRAM_PAGE,
  LOAD_READ_ADDRESS,
  READ_PAGE
} arduinoState_t;

typedef struct {
  arduinoState_t state;
  unsigned int page;
} arduinoUpdateState_t;

void arduinoReset();
bool arduinoBeginUpdate();
char arduinoGetStatus();
void arduinoHandleData(uint8 incoming);
bool arduinoUpdating();
int arduinoPagesFlashed();

#endif