# esp8266-co2
SenseAir S8 CO2 module on an ESP8266 ESP-12E.  The client publishes over MQTT and updates a channel on ThingSpeak.


# Used in Moz-TW
Moz-TW uses ThingSpeak instead of MQTT.  The hardware is a LoLin NodeMcu V3.  The wiring follows,

```
NodeMCU    S8
      G -- GND
     D7 -- TX
     D8 -- RX
     VU -- Vcc (+5V)
```

Real-time CO2 value in MozTW is published on [ThingSpeak](https://thingspeak.com/channels/631210).


# License
ESP8266 Core is licensed under LGPL.  My implementation of S8 ModBus follows MIT license.
