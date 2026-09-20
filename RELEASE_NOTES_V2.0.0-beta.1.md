# OpenRF Platform v2.0.0-beta.1

ESP32-S3 migration release notes.

## Purpose of this release

v2.0.0-beta.1 was the first public beta of the ESP32-S3 generation. Its primary
purpose was to move OpenRF Platform from the memory- and processing-constrained
ESP8266 baseline to the ESP32-S3 N16R8 architecture while preserving the
established gateway features.

Later dual-radio Protocol Engine, bidirectional RAW and Deep Analyzer foundation
work belongs to v2.0.0-beta.2 and is intentionally documented separately.

## Main changes

- Migrated the active firmware target to ESP32-S3 N16R8.
- Enabled the intended 16 MB flash and 8 MB OPI PSRAM configuration.
- Separated radio processing from system and network services using the two CPU
  cores and queue-based communication.
- Moved large working buffers to PSRAM where appropriate while retaining
  time-critical ISR, queue and task data in internal memory.
- Preserved the established WebUI, REST API, MQTT, Home Assistant Discovery,
  OTA firmware update and backup/restore capabilities.
- Preserved RF Learn/Replay, RF Slots, RX Slots and Analyzer functionality as
  the migration baseline.
- Added live CPU, heap and PSRAM monitoring for the new platform.
- Established the architecture on which the later V2 RF subsystem could be built.

## Hardware target

| Area | v2.0.0-beta.1 |
| --- | --- |
| MCU | ESP32-S3 |
| Board configuration | N16R8 |
| Flash | 16 MB |
| PSRAM | 8 MB OPI |
| Framework | Arduino on PlatformIO |
| Filesystem | LittleFS |

## Upgrade boundary

The ESP32-S3 generation is not a drop-in firmware update for ESP8266 hardware.
Use the board definition, partition layout and firmware assets supplied with the
matching release. Back up configuration and saved RF data before replacing a
filesystem image.

## Historical status

This release remains the migration milestone. Development continued in
v2.0.0-beta.2 with two dedicated CC1101 paths, the modular V2 Protocol Engine,
native protocol RX/TX, bidirectional Learned RAW slots, expanded diagnostics and
the initial Deep Analyzer foundations.
