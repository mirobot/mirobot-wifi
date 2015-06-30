#ifndef RBOOT_H
#define RBOOT_H

#include "../rboot/rboot.h"

// get the rboot config
rboot_config ICACHE_FLASH_ATTR rboot_get_config();
bool ICACHE_FLASH_ATTR rboot_set_config(rboot_config *conf);
uint8 ICACHE_FLASH_ATTR rboot_get_current_rom();
bool ICACHE_FLASH_ATTR rboot_set_current_rom(uint8 rom);

#endif