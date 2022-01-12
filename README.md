# NTRIP-Client for M5StickC(ESP32) CAT-M(SIM7080G)

https://github.com/GLAY-AK2/NTRIP-client-for-Arduino
をもとにWiFiClient でなくArduinoHttpClient　を使い
LTE-Mモデム（SIM7080G）の通信でNtripCasterからRTCM補正情報を受信しGPS受信機に送信します。
**まだβ版　数分しか動きません　**

DGPS (Differential GPS) and RTK (GPS positioning with centimeter level accuracy) requires the reference position data.

NTRIP caster relays GNSS reference position data Stream from BaseStation (GNSS reference position is called that) to GNSS rover via NTRIP protocol.
NTRIP client get GNSS reference data from NTRIP casters for high precision receivers via Internet.

This suports request Source Table (NTRIP caster has many BaseStation data. Caster informe all of these. Its List is called "Source Table".) and GNSS Reference Data.

## How to use
Please check  source.

This source has NTRIPClient class based on ArduinoHttpClient 
This Developed on ESP32 core for Arduino.

Source code needs to 
#include <TinyGsmClient.h>
#include <ArduinoHttpClient.h>


### Support
Now, this source check with M5-stickC.
