# ESP8266 Final Release

OpenRF Platform v1.2.0 is the final ESP8266 feature release.

## Gateway Mode

Normal production operation with RX Slots, MQTT, Home Assistant, Learn, TX, OTA and WebUI. The full Analyzer is disabled for stability.

## Developer Mode (Exclusive Analyzer)

The complete RF Analyzer receives exclusive priority. RX Slots, MQTT RF events, Home Assistant Discovery, RF event forwarding and non-essential gateway services are inactive until Developer Mode is disabled.

This limitation is specific to ESP8266. OpenRF Platform ESP32 v2 will support Analyzer and gateway operation simultaneously.

## Maintenance policy

The ESP8266 branch may receive critical stability or security fixes. No new feature development is planned.
