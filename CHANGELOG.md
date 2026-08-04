# Changelog

## v1.2.0 Final — ESP8266 final feature release

- Added Kinetic Protocol Framework and NVKP01 Kinetic support.
- Added NVKP01 Learn/RX Slot/MQTT/Home Assistant event path.
- Added Protocol Manager with classic and Kinetic decoder branches.
- Added 433.920 / 868.350 MHz selection with CC1101/antenna warning.
- Added Gateway Mode and Developer Mode (Exclusive Analyzer).
- Full Analyzer is disabled in Gateway Mode for ESP8266 stability.
- RX Slots, MQTT RF events, Home Assistant Discovery and event forwarding are inactive while Exclusive Analyzer Mode is active.
- Added clear WebUI warnings and ESP32 v2 limitation notice.
- Added gradual Home Assistant Discovery publication.
- Reduced RAW buffer RAM use and added heap health diagnostics.
- Improved Analyzer/API/frontend memory behavior.
- Expanded README and release documentation.
- Declared v1.2.0 the final ESP8266 feature release; future development moves to ESP32 v2.

# v1.2.0 — ESP8266 Final Feature Release

- Added NVKP01 Kinetic protocol support with RX Slots, MQTT and Home Assistant integration.
- Added selectable 433/868 MHz operating band in Settings.
- Added a hardware notice for frequency-specific CC1101 modules and antennas.
- Optimized Analyzer API memory use with bounded RAW previews, direct JSON streaming and slower non-overlapping polling.
- Reduced RAW buffer memory pressure and staggered Home Assistant Discovery publishing for ESP8266 stability.

# Changelog

All notable changes to OpenRF Platform will be documented in this file.

The format is inspired by Keep a Changelog.

---

# v1.1.0 — Analyzer v2 Final

Release date: July 2026

## Added

- Adaptive Candidate diagnostics before Analyzer filtering
- Adjustable Analyzer-only RSSI threshold
- Last Candidate RSSI, pulse count, duration and reject reason
- RAW and normalized RAW pulse views
- Alternation ratio, same-sign pair and longest-run metrics
- Adjustable alternation tolerance
- Visible yellow Developer Mode with ON/OFF state
- Persistent Analyzer settings in LittleFS

## Changed

- Restored the clean v1.0.0 Analyzer layout and original filtering behaviour as Standard Mode
- Moved Analyzer v2 candidate capture, advanced thresholds and normalized diagnostics into Developer Mode
- Improved unknown structured-signal investigation without affecting RX Slots, RF Learn, MQTT or Home Assistant
- Finalized Analyzer v2 UI and diagnostics for public release

## Fixed

- Improved visibility of Candidate rejection causes
- Prevented advanced Analyzer controls from cluttering normal operation

---

# v1.0.0 — First Stable Release

Release date: July 2026

## Added

- Initial public release
- Modular RF Gateway architecture
- RAW Learn
- RAW Replay
- 30 persistent TX slots
- Universal RX Slots
- Native RF protocol decoder framework
- RF Analyzer
- MQTT integration
- Home Assistant Discovery
- REST API
- OTA firmware updates
- Backup / Restore
- WebUI
- LittleFS configuration storage

## Changed

- Project renamed to **OpenRF Platform**
- Stable REST API
- Stable WebUI
- Modular decoder architecture
- PlatformIO project structure

## Removed

- Legacy RAW matcher
- Experimental HCS200 implementation

---

# Previous development

Earlier development milestones are documented inside the `/docs` directory.
### NVKP01 false-trigger protection

Actionable NVKP01 RX Slot, MQTT and Home Assistant events now require two short, structurally valid Kinetic captures within one second. The short-frame envelope (28–70 pulses, 18–60 ms) excludes long RF remote trains, while the second capture corresponds to the mechanical press/release cycle. Analyzer recognition remains intentionally broader for diagnostics.
