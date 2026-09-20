# Changelog

Release-specific implementation detail and the full v2 development history are
kept in the linked release notes. This file lists the user-visible release line.

## v2.0.0-beta.2

- Moved the active platform to ESP32-S3 N16R8 with dual-core tasks and PSRAM-aware memory use.
- Added two-CC1101 concurrent receive support with independent 433/868 MHz profiles,
  frequency scanning, temporary tuning and stored source-frequency metadata.
- Introduced the modular V2 protocol engine and registry, normalized events,
  authoritative routing and protocol-local encoders.
- Added native EV1527/Princeton, PT2262/PT2272 and HT12E receive/transmit support.
- Added receive-only NVKP01 Kinetic recognition with conservative structural gates
  and a sliding 400 ms inactivity dedup window.
- Removed general legacy protocol receive and transmit fallback paths.
- Added bidirectional Learned RAW slots: unchanged full-capture replay plus
  PSRAM-preferred signature matching, runtime statistics, MQTT and HA events.
- Added a dual-radio Analyzer, expanded diagnostics, live core, heap and PSRAM
  gauges, radio hardware controls and recovery diagnostics.
- Added backup/restore, firmware OTA, MQTT and Home Assistant integration for the
  expanded slot/event model.
- Refined the responsive WebUI, restored the original header gauges, corrected
  Dashboard radio state, repaired Diagnostics layout and reduced vertical spacing.
- Phase A FIX3 reports RSSI-filtered Analyzer captures explicitly and uses the
  authoritative V2 Protocol Engine result for accepted Analyzer frames.
- Phase A FIX3.1 lets a newer RSSI-filtered candidate replace an older accepted
  Analyzer result on screen; the most recent RF event now controls the status.
- Phase A FIX4 restores comfortable navigation spacing and removes the legacy
  `Last RF pulse count` and `Last RF RSSI` Home Assistant Discovery sensors.
  Raw `/rx` MQTT diagnostics remain available, while Home Assistant receives
  only actionable protocol, RX Slot and Learned RAW events.
- Consolidated the ESP32-S3 port, subsystem and Step/FIX notes into one beta document.

See [RELEASE_NOTES_V2.0.0-beta.2.md](RELEASE_NOTES_V2.0.0-beta.2.md).

## v2.0.0-beta.1

First public beta of the ESP32-S3 generation:

- Migrated the active firmware platform from ESP8266 to ESP32-S3 N16R8.
- Added dual-core task separation for RF processing and system/network services.
- Added 8 MB PSRAM-aware working memory and 16 MB flash support.
- Preserved the established WebUI, RF Learn/Replay, MQTT, Home Assistant,
  REST API, OTA and backup/restore baseline during the platform migration.
- Established the ESP32-S3 architecture used by subsequent V2 RF development.

See [RELEASE_NOTES_V2.0.0-beta.1.md](RELEASE_NOTES_V2.0.0-beta.1.md).

## v1.2.0

Final ESP8266 feature release and migration baseline for the ESP32-S3 generation.

See [RELEASE_NOTES_V1.2.0.md](RELEASE_NOTES_V1.2.0.md).

## v1.1.0

Expanded RF Analyzer workflows and signal inspection tools.

See [RELEASE_NOTES_V1.1.0.md](RELEASE_NOTES_V1.1.0.md).

## v1.0.0

First stable release with the core RF gateway, WebUI and integration features.

See [RELEASE_NOTES_V1.0.0.md](RELEASE_NOTES_V1.0.0.md).
