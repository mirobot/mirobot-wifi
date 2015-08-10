-include version.mk

#Position and maximum length of espfs in flash memory. This can be undefined. In this case 
#the webpages will be linked in into the executable file. If this is defined, please do a 
#'make htmlflash' to flash the espfs into the ESPs memory.
ESPFS_POS = 0x70000
ESPFS_SIZE = 0x6000

# Output directors to store intermediate compiled files
# relative to the project directory
BUILD_BASE	 = build
FW_BASE		   = firmware
RELEASE_BASE = release

# Base directory for the compiler. Needs a / at the end; if not set it'll use the tools that are in
# the PATH.
XTENSA_TOOLS_ROOT ?= 

# base directory of the ESP8266 SDK package, absolute
SDK_BASE	?= /opt/Espressif/ESP8266_SDK

#Esptool.py path and port
ESPTOOL		?= esptool.py
ESPTOOL2	?= esptool2
ESPPORT		?= /dev/ttyUSB0
#ESPDELAY indicates seconds to wait between flashing the two binary images
ESPDELAY	?= 3
ESPBAUD		?= 460800


# name for the target project
TARGET		= httpd

# which modules (subdirectories) of the project to include in compiling
MODULES		= driver user
EXTRA_INCDIR	= include libesphttpd/include libesphttpd/core

# libraries used in this project, mainly provided by the SDK
LIBS		= c gcc hal phy pp net80211 wpa main lwip
#Add in esphttpd lib
LIBS += esphttpd

# compiler flags using during compilation of source files
CFLAGS		= -Os -ggdb -std=c99 -Werror -Wpointer-arith -Wundef -Wall -Wl,-EL -fno-inline-functions \
		-nostdlib -mlongcalls -mtext-section-literals  -D__ets__ -DICACHE_FLASH -D_STDINT_H \
		-Wno-address -DVERSION=\"$(VERSION)\"

# linker flags used to generate the main object file
LDFLAGS		= -nostdlib -Wl,--no-check-sections -u call_user_start -Wl,-static

# linker script used for the above linkier step
LD_SCRIPT	= eagle.app.v6.ld

# various paths from the SDK used in this project
SDK_LIBDIR	= lib
SDK_LDDIR	= ld
SDK_INCDIR	= include

# select which tools to use as compiler, librarian and linker
CC		:= $(XTENSA_TOOLS_ROOT)xtensa-lx106-elf-gcc
AR		:= $(XTENSA_TOOLS_ROOT)xtensa-lx106-elf-ar
LD		:= $(XTENSA_TOOLS_ROOT)xtensa-lx106-elf-gcc


####
#### no user configurable options below here
####
SRC_DIR		:= $(MODULES)
BUILD_DIR	:= $(addprefix $(BUILD_BASE)/,$(MODULES))

SDK_LIBDIR	:= $(addprefix $(SDK_BASE)/,$(SDK_LIBDIR))
SDK_INCDIR	:= $(addprefix -I$(SDK_BASE)/,$(SDK_INCDIR))

SRC		:= $(foreach sdir,$(SRC_DIR),$(wildcard $(sdir)/*.c))
OBJ		:= $(patsubst %.c,$(BUILD_BASE)/%.o,$(SRC))
LIBS		:= $(addprefix -l,$(LIBS))
APP_AR		:= $(addprefix $(BUILD_BASE)/,$(TARGET)_app.a)
TARGET_OUT	:= $(addprefix $(BUILD_BASE)/,$(TARGET).out)

LD_SCRIPT	:= $(addprefix -T$(SDK_BASE)/$(SDK_LDDIR)/,$(LD_SCRIPT))

INCDIR	:= $(addprefix -I,$(SRC_DIR))
EXTRA_INCDIR	:= $(addprefix -I,$(EXTRA_INCDIR))
MODULE_INCDIR	:= $(addsuffix /include,$(INCDIR))

V ?= $(VERBOSE)
ifeq ("$(V)","1")
Q :=
vecho := @true
else
Q := @
vecho := @echo
endif

ifeq ("$(GZIP_COMPRESSION)","yes")
CFLAGS		+= -DGZIP_COMPRESSION
endif

ifeq ("$(USE_HEATSHRINK)","yes")
CFLAGS		+= -DESPFS_HEATSHRINK
endif

ifeq ("$(ESPFS_POS)","")
#No hardcoded espfs position: link it in with the binaries.
LIBS += -lwebpages-espfs
else
#Hardcoded espfs location: Pass espfs position to rest of code
CFLAGS += -DESPFS_POS=$(ESPFS_POS) -DESPFS_SIZE=$(ESPFS_SIZE)
endif

vpath %.c $(SRC_DIR)

define compile-objects
$1/%.o: %.c
	$(vecho) "CC $$<"
	$(Q) $(CC) $(INCDIR) $(MODULE_INCDIR) $(EXTRA_INCDIR) $(SDK_INCDIR) $(CFLAGS)  -c $$< -o $$@
endef

.PHONY: all checkdirs clean libesphttpd

all: checkdirs $(TARGET_OUT) $(FW_BASE)

libesphttpd/Makefile:
	$(Q) echo "No libesphttpd submodule found. Using git to fetch it..."
	$(Q) git submodule init
	$(Q) git submodule update

html/index.html: $(wildcard web/*/*)
	$(vecho) "Squash $@"
	$(Q) VERSION=$(VERSION) ./bin/shrink_html.rb

libesphttpd/libesphttpd.a: libesphttpd/Makefile html/index.html
	$(vecho) "MAKE $@"
	$(Q) make -C libesphttpd

rboot:
	$(Q) make -C rboot

$(TARGET_OUT): $(APP_AR)
	$(vecho) "LD $@"
	$(Q) $(LD) -Llibesphttpd -L$(SDK_LIBDIR) -Tld/rom0.ld $(LDFLAGS) -Wl,--start-group $(LIBS) $(APP_AR) -Wl,--end-group -o $(BUILD_BASE)/mirobot.rom0.out
	$(Q) $(LD) -Llibesphttpd -L$(SDK_LIBDIR) -Tld/rom1.ld $(LDFLAGS) -Wl,--start-group $(LIBS) $(APP_AR) -Wl,--end-group -o $(BUILD_BASE)/mirobot.rom1.out

$(FW_BASE): $(TARGET_OUT) rboot
	$(vecho) "FW $@"
	$(Q) mkdir -p $@
	$(Q) cp libesphttpd/webpages.espfs $(FW_BASE)/mirobot-v2-ui.bin
	$(Q) cp rboot/firmware/rboot.bin $(FW_BASE)/rboot.bin
	$(Q) $(ESPTOOL2) -quiet -bin -boot2 $(BUILD_BASE)/mirobot.rom0.out $(FW_BASE)/mirobot-v2-wifi.rom0.bin .text .data .rodata
	$(Q) $(ESPTOOL2) -quiet -bin -boot2 $(BUILD_BASE)/mirobot.rom1.out $(FW_BASE)/mirobot-v2-wifi.rom1.bin .text .data .rodata

$(APP_AR):  libesphttpd/libesphttpd.a $(OBJ)
	$(vecho) "AR $@"
	$(Q) $(AR) cru $@ $(OBJ)

checkdirs: $(BUILD_DIR)  

$(RELEASE_BASE): $(FW_BASE)
	$(Q) mkdir -p $@
	$(Q) cp $(FW_BASE)/mirobot-v2-ui.bin $(RELEASE_BASE)/mirobot-v2-ui-$(VERSION).bin
	$(Q) cp $(FW_BASE)/mirobot-v2-wifi.rom0.bin $(RELEASE_BASE)/mirobot-v2-wifi-$(VERSION).rom0.bin
	$(Q) cp $(FW_BASE)/mirobot-v2-wifi.rom1.bin $(RELEASE_BASE)/mirobot-v2-wifi-$(VERSION).rom1.bin
	
$(BUILD_DIR):
	$(Q) mkdir -p $@

flash: $(TARGET_OUT) $(FW_BASE)
	$(Q) $(ESPTOOL) --port $(ESPPORT) --baud $(ESPBAUD) write_flash 0x00000 $(FW_BASE)/0x00000.bin 0x40000 $(FW_BASE)/0x40000.bin

flashweb: $(FW_BASE)
	$(vecho) "curl --request POST --data-binary @firmware/mirobot-v2-ui.bin http://$(IP)/admin/updateui.cgi"

flashwebfw: $(FW_BASE)
	$(Q) ./bin/flashfw.rb $(IP)

blankflash:
	$(Q) $(ESPTOOL) --port $(ESPPORT) --baud $(ESPBAUD) write_flash 0x7E000 $(SDK_BASE)/bin/blank.bin

testwebsize: libesphttpd/libesphttpd.a
	$(Q) if [ $$(stat -f '%z' firmware/mirobot-v2-ui.bin) -gt $$(( $(ESPFS_SIZE) )) ]; then echo "webpages.espfs too big!"; false; fi

htmlflash: libesphttpd/libesphttpd.a testwebsize
	$(Q) $(ESPTOOL) --port $(ESPPORT) --baud $(ESPBAUD) write_flash $(ESPFS_POS) libesphttpd/webpages.espfs

flashall: libesphttpd/libesphttpd.a $(TARGET_OUT) $(FW_BASE) testwebsize
	$(Q) $(ESPTOOL) --port $(ESPPORT) --baud $(ESPBAUD) write_flash 0x00000 firmware/rboot.bin 0x01000  $(SDK_BASE)/bin/blank.bin 0x2000 firmware/mirobot-v2-wifi.rom0.bin $(ESPFS_POS) firmware/mirobot-v2-ui.bin

clean:
	$(Q) make -C libesphttpd clean
	$(Q) rm -f $(APP_AR)
	$(Q) rm -f $(TARGET_OUT)
	$(Q) find $(BUILD_BASE) -type f | xargs rm -f
	$(Q) rm -rf $(FW_BASE)
	$(Q) rm -rf $(RELEASE_BASE)
	$(Q) rm -rf html

$(foreach bdir,$(BUILD_DIR),$(eval $(call compile-objects,$(bdir))))
