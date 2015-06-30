Mirobot-WiFi
============

This is the code that runs on the WiFi module on the V2 Mirobot. It is an ESP8266 based module (the ESP-12) and you should build this project using the esp-open-sdk toolchain (https://github.com/pfalcon/esp-open-sdk)

It has a number of features:
 - A WebSocket to serial server
 - The ability to reflash an Arduino from the chip without running the STK500 protocol over the air
 - Over the Air upgrades using rboot
 - A built-in web server, thanks to Jeroen Domburg for starting that project
 - More coming soon

It uses a custom memory map to enable all of the different firmware updates (main firmware, Arduino and web pages):

Location | Size (kB) | Use
---------|-----------|-------------------
0x00000  | 4	       | Bootloader
0x01000	 | 4	       | Bootloader config
0x02000	 | 220     	 | rom0
0x39000	 | 220	     | rom1
0x70000	 | 24	       | UI Files
0x76000	 | 32	       | Arduino cache
0x7E000	 | 8	       | WiFi configuration

It's still in pre-release stage so use at your own risk