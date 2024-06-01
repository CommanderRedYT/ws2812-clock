# ws2812-clock

This is a project I started when a teacher told us to do a "simple" project. This project is a PCB containing 232
WS2812B LEDs that are controlled via a ESP32. The current version (v2) does not have a RTC module and entirely relies on
NTP. The PCB comes with a USB-C port which you can also use to program the port (No power delivery tho, only normal USB 5V).
The PCB also has a barrel jack that has reverse polarity protection using a P-FET. I also added a footprint for both a
SHT-30 and a BME-280 sensor. However, with how warm the LEDs get with high brightness, the temperature reading are not
accurate if you are looking for the room temperature.

## Features
- 232 WS2812B LEDs
- ESP32
- USB-C with programmer (CH343)
- Barrel jack with reverse polarity protection
