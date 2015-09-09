#ifndef Sha1_h
#define Sha1_h

#include "c_types.h"

/* header */

#define HASH_LENGTH 20
#define BLOCK_LENGTH 64

typedef struct sha1nfo {
  uint32 buffer[BLOCK_LENGTH/4];
  uint32 state[HASH_LENGTH/4];
  uint32 byteCount;
  uint8 bufferOffset;
  uint8 keyBuffer[BLOCK_LENGTH];
  uint8 innerHash[HASH_LENGTH];
} sha1nfo;

/* public API - prototypes - TODO: doxygen*/

/**
 */
void sha1_init(sha1nfo *s);
/**
 */
void sha1_writebyte(sha1nfo *s, uint8 data);
/**
 */
void sha1_write(sha1nfo *s, const char *data, uint8 len);
/**
 */
uint8* sha1_result(sha1nfo *s);
/**
 */
void sha1_initHmac(sha1nfo *s, const uint8* key, int keyLength);
/**
 */
uint8* sha1_resultHmac(sha1nfo *s);

#endif
