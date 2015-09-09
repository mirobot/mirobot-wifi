#ifndef WS_H
#define WS_H
#include <c_types.h>
#include <espconn.h>

typedef struct WsPriv WsPriv;
typedef struct WsConnData WsConnData;

typedef enum {
  UNKNOWN,
  RAW,
  WEBSOCKET
} connType_t;

typedef enum {
  WAITING_FOR_SEC,
  INITIALISED
} webSocketState_t;

//A struct describing a http connection. This gets passed to cgi functions.
struct WsConnData {
  struct espconn *conn;
  WsPriv *priv;
  char *url;
  char *key;
  connType_t connType;
  webSocketState_t webSocketState;
};

void ICACHE_FLASH_ATTR wsInit(int port, void *handler);
void ICACHE_FLASH_ATTR wsSend(char *msg);

#endif