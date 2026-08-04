# ESP8266 Final Analyzer Guard

This final stability profile intentionally limits the live Analyzer on ESP8266.

- Analyzer polling: 2 seconds
- General RAW polling pauses while Analyzer is open
- Normal RAW preview: 24 pulses
- Developer RAW preview: 48 pulses
- Analyzer API pauses below 17 kB free heap or 9 kB largest block
- RF reception, RX Slots, MQTT and Home Assistant continue running when the Analyzer API is paused

This is the final ESP8266 boundary. Richer live diagnostics continue on ESP32 v2.
