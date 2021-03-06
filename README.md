#temp\_humid\_combined\_mqtt#

By Dan Gentry, 2021

I've built many home environment sensors and written many different programs to drive them, but they all seem to do the same thing.  So I decided to write one program to run them all.

This version can handle three different common sensors, and could be augmented to cover more. Also, I have included code to use MQTT discovery with Home Assistant, which could probably be modified to work with other platforms.

While this has only been tested with an ESP8266 (Wemos D1 Mini), I am confident that an ESP32 could also be used.  If changes are needed, please let me know.

##Notes:##

My go to hardware is a Wemos D1 Mini with an SHT-30 shield.  When atmospheric pressure is also needed, I've substituted the BMP/BME 280.  The DHT-22 is also supported.  With both the SHT and BMP, I have used the default SCL and SDA pins.  You'll have to make modifications if other pins are used.  The DHT-22 data wire is normally connected to D4, but I don't know what pin a DHT-22 shield would use.

A 220K resistor is connected between the 5V pin and A0 to measure the voltage of the power supply.  Version 2 of the D1 Mini Pro uses a jumper instead to make this connection.

On some builds, I have included a lithium ion battery with a solar panel to charge it.  For these the resistor is connected to battery positive instead of the 5V pin.

MQTT discovery for Home Assistant is also included.  If discovery is enabled on the Hass server, the device will automatically show up in the device list when it powers up.

##config.h:##

Copy the config.example file as config.h, them modify the settings to your needs.

1. OTA stands for Over The Air updates.  With this path and user credentials, one an upload a fresh .bin file for the project.  The path in our example would be: http://office.local/WebFirmwareUpgrade.
1. The MQTT settings are pretty straight forward.  Change <host> to the prefix you wish to use, and remember that it must unique for your system.
1. For Sensor Type and Power Source, use the constants defined just above to indicate the sensor and power source in use.  An example:
**#define CONFIG\_SENSOR\_TYPE SHT_3X**

##To Do:##

1. I don't have all of the battery types configured yet to give the best percentage of max voltage.
1. The MQTT availability and Last Will and Testement code is present, but commented becuase it is not used with deep sleep.  It is there for documentation purposes, but I should probably take it out.
1. The CONFIG_DEBUG functionality is not yet implemented.
1. The MQTT discovery messages were being truncated, so I had to temporarily remove some of the attributes.  Still working on that.
1. The SHT-3X i2c address is hardcoded at 0x45.  0x44 is the default for a free standing development board, but the D1 Mini shield uses 0x45.
