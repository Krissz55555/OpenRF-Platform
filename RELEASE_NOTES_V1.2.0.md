# OpenRF Platform v1.2.0 Final

## Final feature release for ESP8266

OpenRF Platform v1.2.0 is the final feature release for the ESP8266 edition. It closes the first hardware generation with a documented and stable feature set. Future feature development continues on OpenRF Platform ESP32 v2. The ESP8266 branch may still receive critical fixes.

## Highlights

### Kinetic RF

- Added the Kinetic Protocol Framework.
- Added Protocol Manager routing for classic and Kinetic decoders.
- Added NVKP01 Kinetic recognition and actionable `PRESS` events.
- Added RX Slot, MQTT and Home Assistant support for NVKP01.
- Kept protocol recognition separate from actionable event decoding.

### Two ESP8266 operating modes

**Gateway Mode** is the default production mode. RX Slots, MQTT, Home Assistant, Learn, TX and normal gateway services are active. The full Analyzer is disabled to maximize stability.

**Developer Mode (Exclusive Analyzer)** enables the complete Analyzer at full speed. While active, RX Slot processing, MQTT RF events, Home Assistant Discovery, RF event forwarding and non-essential gateway services are temporarily inactive. The UI displays a prominent warning and explains that this restriction applies only to ESP8266.

OpenRF Platform ESP32 v2 will support Analyzer and gateway functions simultaneously without this limitation.

### 433 / 868 MHz selection

- Added Settings selection for 433.920 MHz and 868.350 MHz.
- Selection is stored in LittleFS and applied after restart.
- Added hardware warning: use a CC1101 module and antenna designed or tuned for the selected band.

### Stability and memory

- Reduced RAW buffer RAM footprint while retaining margin above the accepted frame limit.
- Added heap, minimum-heap, maximum-block and fragmentation health logging.
- Added gradual Home Assistant Discovery publishing.
- Reduced simultaneous JSON, HTTP, MQTT and LittleFS memory pressure.
- Added exclusive Analyzer operation to prevent ESP8266 heap corruption under combined workloads.
- Improved API and frontend consistency.

### Documentation

- Reworked README into the complete entry-point documentation.
- Documented installation, wiring, upload behavior, Learn, TX/RX Slots, MQTT, Home Assistant, REST API, Backup/Restore, Analyzer, Kinetic RF, frequency selection and troubleshooting.
- Documented the end of ESP8266 feature development and the ESP32 v2 transition.

## Upgrade notes

- Upload the firmware image.
- Upload the LittleFS image because the WebUI changed.
- Back up configuration first: filesystem upload can replace Wi-Fi, MQTT, Analyzer and slot data.
- Restore configuration after the filesystem upload if required.
- Recheck the selected 433/868 MHz band after migration.

## Validation focus

Before public release, test:

- Multi-day Gateway Mode stability
- Developer Mode Analyzer stability
- Switching between Gateway and Developer modes
- NVKP01 → RX Slot → MQTT → Home Assistant
- 433/868 setting persistence across restart
- Backup/Restore
- OTA
- Discovery without duplicated Home Assistant entities

## Platform status

ESP8266 v1.2.0 is feature-complete. New feature work moves to ESP32 v2, including dual-radio 433/868 operation and unrestricted concurrent Analyzer/gateway use.

### NVKP01 false-trigger protection

Actionable NVKP01 RX Slot, MQTT and Home Assistant events now require two short, structurally valid Kinetic captures within one second. The short-frame envelope (28–70 pulses, 18–60 ms) excludes long RF remote trains, while the second capture corresponds to the mechanical press/release cycle. Analyzer recognition remains intentionally broader for diagnostics.
