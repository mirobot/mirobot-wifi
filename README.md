Mirobot-WiFi
============

This is the code that runs on the WiFi module on the V2 Mirobot. It is an ESP8266 based module (the ESP-01) and you should build this project using the esp-open-sdk toolchain (https://github.com/pfalcon/esp-open-sdk)

It has a number of features:
 - A WebSocket to serial server
 - The ability to reflash an Arduino from the chip without running the STK500 protocol over the air
 - A built-in web server, thanks to Jeroen Domburg for starting that project
 - More coming soon
 
It's still in pre-release stage so use at your own risk