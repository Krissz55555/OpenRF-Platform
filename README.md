# OpenRF Platform

<p align="center">
  <img src="assets/openrf-platform-logo.svg" width="320" alt="OpenRF Platform Logo">
</p>

<h1 align="center">OpenRF Platform</h1>

<p align="center"><strong>Open-source RF gateway and protocol-development platform for ESP8266 + CC1101.</strong></p>

<p align="center">RF Gateway • RF Analyzer • Kinetic RF • MQTT • Home Assistant • REST API • OTA • 433 / 868 MHz</p>

---

## Key technologies

- ESP8266 (NodeMCU V2)
- CC1101 RF transceiver
- RadioLib
- PlatformIO
- LittleFS
- MQTT
- Home Assistant Discovery
- REST / JSON API
- OTA firmware updates

---

## Current release

**OpenRF Platform v1.2.0 Final**

Version 1.2.0 is the **final feature release for ESP8266**. The ESP8266 edition is now feature-complete and will receive only critical stability or security fixes when required. All future feature development continues on **OpenRF Platform ESP32 v2**.

OpenRF Platform began as a practical 433 MHz remote-control project and evolved into a modular RF platform with protocol-aware reception, RAW learning and replay, MQTT, native Home Assistant integration, a browser-based RF Analyzer, Kinetic RF support, OTA updates and persistent configuration.

The design principle remains:

> **Keep the platform stable while allowing RF protocols to evolve independently.**

---

## Major features

### RF gateway

- CC1101 OOK/RAW reception and transmission
- 30 persistent RAW TX slots
- RAW Learn, preview, accept, save and replay
- Universal protocol-aware RX Slots
- Repeat lockout and automation-safe event handling
- 433.920 MHz or 868.350 MHz operating-band selection

### Native protocol framework

- Modular Protocol Manager
- Classic decoder branch
- Kinetic decoder branch
- EV1527 / HS1527 / Princeton
- PT2262 / PT2272-style tri-state signals
- NVKP01 Kinetic support

### Kinetic RF support

v1.2.0 introduces the first native Kinetic RF integration:

- NVKP01 protocol recognition
- Stable Kinetic `PRESS` event generation
- RX Slot learning
- MQTT event publication
- Home Assistant Discovery and automation triggers
- Recognition separated from actionable decoding
- Multi-stage event confirmation reduces false-positive detections in noisy RF environments

The current NVKP01 implementation intentionally treats the complete mechanical action as one button event. Press and release telegrams are not exposed separately because that distinction has no practical benefit for the supported one-button device. The framework remains extensible if future devices require separate events.

### MQTT and Home Assistant

- Configurable MQTT broker and base topic
- Retained Home Assistant Discovery
- Gradual discovery publication to reduce ESP8266 memory pressure
- TX controls, RX binary sensors and device automation triggers
- Stable identifiers to avoid duplicated Home Assistant entities
- Event payloads containing protocol, device, control, action, quality and RSSI data

### Web interface

- Dashboard
- RF Learn
- TX Slots
- RX Slots
- RF Analyzer
- Settings
- System diagnostics
- OTA Update
- Backup and Restore

### System functions

- Wi-Fi Station mode
- First-start Access Point configuration
- LittleFS persistent storage
- REST/JSON API
- OTA firmware update
- Configuration and slot backup/restore
- Runtime heap, largest-block and fragmentation diagnostics

---

## ESP8266 operating modes

The full RF Analyzer and the complete gateway workload cannot run concurrently with adequate long-term stability on ESP8266. Version 1.2.0 therefore provides two explicit operating modes.

### Gateway Mode — default

Gateway Mode is intended for everyday operation.

Active services:

- RF reception and transmission
- RAW Learn and replay
- Universal RX Slots
- MQTT
- Home Assistant Discovery and events
- Web interface
- OTA and configuration services

The full RF Analyzer is **disabled** in Gateway Mode to maximize available memory and prevent heap fragmentation. The Analyzer page clearly indicates that diagnostics are inactive and provides access to Developer Mode.

### Developer Mode (Exclusive Analyzer)

Developer Mode is intended exclusively for RF protocol analysis and debugging.

Active services:

- Full RF Analyzer v2
- Fast Analyzer updates
- RAW and normalized RAW previews
- Pulse classes and timing diagnostics
- Candidate analysis
- Similarity and occurrence analysis
- Alternation statistics
- Reject reasons
- Protocol recognition diagnostics

While Developer Mode is active, the following gateway functions are temporarily inactive:

- RX Slot processing
- MQTT RF event publishing
- Home Assistant Discovery
- RF event forwarding
- Non-essential background gateway services

Wi-Fi and the WebUI remain available so the Analyzer can operate. Disable Developer Mode to return immediately to normal Gateway Mode.

> **ESP8266-only limitation:** OpenRF Platform ESP32 v2 will support full Analyzer diagnostics and normal gateway operation simultaneously without this restriction.

---

## RF Analyzer v2

The Analyzer is the protocol-development foundation of OpenRF Platform. It converts unknown RF traffic into measurable and reproducible data before a decoder is implemented.

Developer Mode provides:

- Adjustable Analyzer RSSI threshold
- Candidate capture and freeze
- Pulse count, duration and RSSI
- Pulse min/average/max
- Pulse-class estimation
- Base-pulse and class-ratio diagnostics
- Alternation ratio
- Same-sign pair count
- Longest same-sign run
- RAW normalization
- Structured Unknown detection
- Similarity and occurrence clustering
- Reject-reason diagnostics
- Known-protocol recognition
- Copyable RAW output for decoder development

Analyzer settings are persisted in LittleFS. The Analyzer does not change RF Learn storage, TX slot data, RX slot definitions, MQTT configuration or Home Assistant identifiers.

---

## Supported hardware

| Hardware | Status |
|---|---|
| ESP8266 NodeMCU V2 / NodeMCU 1.0 | Supported — final platform release |
| ESP-12E-based boards | Supported when pin mapping and flash layout match |
| CC1101 433 MHz module | Supported |
| CC1101 868 MHz module | Supported |
| ESP32 | Next-generation v2 development |

### Reference wiring

| CC1101 | ESP8266 NodeMCU V2 |
|---|---|
| VCC | 3V3 |
| GND | GND |
| SCK | GPIO14 / D5 |
| MISO | GPIO12 / D6 |
| MOSI | GPIO13 / D7 |
| CSN | GPIO15 / D8 |
| GDO0 | GPIO4 / D2 |
| GDO2 | Not connected |

Only GDO0 is used for RAW edge capture.

### 433 / 868 MHz selection

Select the operating band on the **Settings** page:

- 433 MHz → 433.920 MHz
- 868 MHz → 868.350 MHz

The selected value is stored in LittleFS and applied after restart.

> **Hardware requirement:** Software frequency selection does not retune the RF hardware. For reliable range and legal operation, use a CC1101 module and antenna designed or tuned for the selected frequency band. Replacing the module and/or antenna may be necessary when changing between 433 MHz and 868 MHz.

---

## Installation

### Requirements

- ESP8266 NodeMCU V2-compatible board
- Suitable CC1101 module
- Correctly tuned antenna
- PlatformIO
- USB data cable

### Build and upload

1. Open the project folder in VS Code with PlatformIO.
2. Verify the board environment in `platformio.ini`.
3. Run **Build**.
4. Run **Upload** for the firmware.
5. Run **Upload Filesystem Image** when the WebUI files in `data/` have changed.

> Uploading the filesystem image replaces LittleFS and may erase Wi-Fi, MQTT, Analyzer and slot configuration. Create a Backup before a filesystem upload and Restore it afterwards when preserving existing configuration is required.

### First start

1. Connect to the `OpenRF-Platform` setup access point.
2. Open `http://192.168.4.1`.
3. Configure Wi-Fi and MQTT.
4. Save the configuration and allow the device to restart.
5. Open the assigned LAN IP address.

---

## RF Learn and TX Slots

RAW Learn records an RF pulse train without requiring protocol knowledge.

Workflow:

```text
Start Learn → transmit the original remote signal → inspect preview → accept → save to TX slot → replay
```

TX slots store RAW pulse sequences in LittleFS. RAW replay is intended for devices you own or are authorized to test. Rolling-code compatibility is not guaranteed.

---

## Universal RX Slots

RX Slots store decoded event identity rather than depending on exact RAW equality.

Typical identity fields:

- Protocol
- Device ID or transmitter signature
- Control / button
- Event / code

For NVKP01:

```text
Protocol: NVKP01 Kinetic
Device: nvkp01
Control: button
Event: PRESS
```

This allows different valid captures of the same physical action to trigger one stable automation event.

---

## MQTT and Home Assistant

OpenRF Platform publishes TX/RX state and events under the configured MQTT base topic. Home Assistant Discovery creates supported entities and device triggers automatically.

Discovery is published gradually on ESP8266 to avoid large simultaneous heap allocations. Stable discovery identifiers prevent entity duplication across restarts and configuration updates.

After changing MQTT settings or slot definitions, allow the discovery sequence to complete before evaluating Home Assistant entities.

---

## REST API

The WebUI uses the same JSON API available to integrations. Endpoint groups cover:

- System and health
- Radio state
- Settings
- RF Learn
- TX Slots
- RX Slots
- Analyzer
- OTA
- Backup and Restore

API consumers should tolerate optional diagnostics fields because Developer Mode and Gateway Mode expose different Analyzer detail levels.

---

## Backup and Restore

Use Backup before:

- Uploading a new LittleFS image
- Migrating configuration
- Testing a release candidate
- Replacing hardware

A backup should preserve supported configuration and slot data. Always verify Restore on the same firmware generation before relying on it for migration.

---

## Troubleshooting

### Dashboard shows API error

- Confirm the device IP address after restart.
- Confirm that firmware and LittleFS WebUI belong to the same release.
- Perform a hard browser refresh (`Ctrl+F5`).
- Re-upload the filesystem image when frontend files were not updated.

### Settings were erased after filesystem upload

Uploading the filesystem image replaces LittleFS. Restore a backup or configure Wi-Fi, MQTT and slots again.

### Device restarts while Analyzer is open

- Confirm that Developer Mode is being used for full Analyzer diagnostics.
- Do not run full Analyzer diagnostics in Gateway Mode.
- Close unnecessary browser tabs connected to the device.
- Check the serial health output for free heap, maximum free block and fragmentation.

### 868 MHz selected but range is poor

Use a CC1101 module and antenna designed for 868 MHz. A 433 MHz antenna or module variant may operate poorly even when the CC1101 frequency register is changed correctly.

### Home Assistant entities appear duplicated

- Allow gradual Discovery to finish.
- Avoid changing the MQTT base topic or hostname unnecessarily.
- Remove obsolete retained discovery topics only when identifiers were deliberately changed.

---

## Project architecture

```text
RF signal
   ↓
CC1101 RAW capture
   ↓
Protocol Manager
   ├── Classic decoders
   └── Kinetic decoders
   ↓
Decoded RF event
   ↓
Universal RX Slot
   ├── MQTT
   └── Home Assistant
```

Protocol recognition and actionable event decoding are separate states. A decoder may identify a protocol for Analyzer diagnostics before it is allowed to generate RX Slot or MQTT events.

---

## Repository documentation

The repository contains detailed technical and historical documents:

- `RELEASE_NOTES_V1.2.0.md` — final ESP8266 release notes
- `CHANGELOG.md` — version changes
- `ANALYZER_V2_FINAL.md` — Analyzer architecture and behavior
- `KINETIC_PROTOCOL_FRAMEWORK.md` — Kinetic decoder architecture
- `PROTOCOL_MANAGER.md` — decoder dispatch model
- `NVKP01_KINETIC_TEST.md` — NVKP01 validation notes
- `ESP8266_STABILITY_TEST.md` — stability and heap-testing notes
- `docs/` — implementation history and subsystem documentation
- `CONTRIBUTING.md` — contribution rules
- `LICENSE` — MIT License

---

## ESP8266 final release policy

OpenRF Platform v1.2.0 completes the ESP8266 edition.

The ESP8266 branch remains available and may receive critical bug fixes, but no additional features are planned. New protocols, dual-radio operation, unrestricted Analyzer operation and further platform expansion belong to ESP32 v2.

This is a deliberate platform transition, not abandonment: v1.2.0 preserves a complete, documented and usable ESP8266 gateway while the proven architecture moves to more capable hardware.

---

## OpenRF Platform ESP32 v2

ESP32 v2 will build directly on the existing platform architecture and is planned to provide:

- Analyzer and gateway services running simultaneously
- Two CC1101 radios
- Parallel 433 MHz and 868 MHz operation
- Larger RAW buffers
- Expanded protocol library
- More Kinetic devices
- Improved diagnostics and WebUI
- Greater memory and processing headroom

The ESP32 edition is the next generation of the same OpenRF Platform ecosystem.

---

## Responsible use

Use OpenRF Platform only with devices you own or are authorized to test. Follow local radio regulations, permitted frequency bands, power limits and duty-cycle requirements. Do not use the project to bypass security systems or interfere with other radio users.

---

## License

OpenRF Platform is released under the MIT License. See [LICENSE](LICENSE).

## Credits

Created and maintained by **Kocsis Krisztián**.

Developed by Kocsis Krisztián with implementation assistance, architecture discussions, and documentation support from **ChatGPT (OpenAI)**.

<p align="center">⭐ If OpenRF Platform is useful to you, consider starring the project on GitHub.</p>
