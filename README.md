# OpenRF Platform

**Open-source RF gateway and protocol-development platform for ESP32-S3 + CC1101.**

RF Gateway • RF Analyzer • Kinetic RF • MQTT • Home Assistant • REST API • OTA • 433 / 868 MHz

---

## Current release

**OpenRF Platform v2.0.0-beta.1 – ESP32-S3**

This is the first public beta of the ESP32-S3 generation of OpenRF Platform.

The ESP32-S3 version is based on the proven v1.2.0 ESP8266 feature set, but removes the main hardware limitations of the original platform through dual-core operation, additional memory and PSRAM-aware buffering.

The ESP8266 implementation remains available in the **`esp8266`** branch and its final stable release remains **v1.2.0**.

> The ESP32-S3 port is now the default branch and the active development platform.

---

## Key technologies

* ESP32-S3 N16R8
* 16 MB Flash
* 8 MB PSRAM
* CC1101 RF transceiver
* RadioLib
* PlatformIO
* LittleFS
* FreeRTOS dual-core tasks
* MQTT
* Home Assistant Discovery
* REST / JSON API
* OTA firmware updates

---

## Major features

### RF Gateway

* CC1101 OOK/RAW reception and transmission
* 30 persistent RAW TX slots
* RAW Learn, preview, accept, save and replay
* Universal protocol-aware RX Slots
* Adjustable RX Slot Learn RSSI filtering
* Repeat lockout and automation-safe event handling
* 433.920 MHz or 868.350 MHz operating-band selection

### Native protocol framework

* Modular Protocol Manager
* Classic decoder branch
* Kinetic decoder branch
* EV1527 / HS1527 / Princeton
* PT2262 / PT2272-style tri-state signals
* NVKP01 Kinetic support

### Kinetic RF support

* NVKP01 protocol recognition
* Stable Kinetic `PRESS` event generation
* RX Slot learning
* MQTT event publication
* Home Assistant Discovery and automation triggers
* Recognition separated from actionable decoding
* Multi-stage confirmation to reduce false positives in noisy RF environments

### MQTT and Home Assistant

* Configurable MQTT broker and base topic
* Home Assistant Discovery
* TX controls
* RX binary sensors
* Device automation triggers
* Stable identifiers
* Event payloads with protocol, device, control, action, quality and RSSI data

### Web interface

* Dashboard
* RF Learn
* RF Slots
* RX Slots
* RF Analyzer
* Settings
* System diagnostics
* About
* OTA Update
* Backup and Restore

### System functions

* Wi-Fi Station mode
* First-start Access Point configuration
* LittleFS persistent storage
* REST / JSON API
* OTA firmware update
* Configuration and slot backup/restore
* Flash, PSRAM and internal heap diagnostics
* Core 0 / Core 1 live load monitoring

---

## OpenRF Platform v2 – Development direction

With the ESP32-S3 port complete, development is moving beyond simply reproducing the ESP8266 feature set on faster hardware.

The primary direction for **OpenRF Platform v2** is to evolve from an RF gateway into a more autonomous **Sub-GHz signal analysis platform**.

The goal is for OpenRF to progressively reduce the amount of RF knowledge required from the user:

> **Press the remote. Let OpenRF find the signal, analyze it, optimize reception, and try to make it usable in Home Assistant.**

### Next stage – Dual CC1101 / 433 + 868 MHz

The next major hardware step is support for a **second CC1101**, allowing dedicated 433 MHz and 868 MHz radios.

Rather than treating them as two independent RF systems, both radios will feed a common RF engine and a unified capture format.

This architecture is intended to provide the foundation for automatic radio selection, signal scanning and deeper RF analysis.

### Deep Analyzer

The **Deep Analyzer** is the main planned feature direction for OpenRF Platform v2.

Instead of only displaying captured RF data, it is intended to compare multiple captures and derive useful information from them.

Planned analysis includes:

* frequency and RSSI characteristics
* noise level
* pulse-width distribution and timing
* estimated bit timing
* frame length and frame gaps
* repeated-frame detection
* preamble and synchronization patterns
* static and changing payload regions
* capture similarity and consistency
* modulation estimation
* static-code detection
* potential rolling/dynamic-code detection

### Automatic CC1101 optimization

A later stage of the Deep Analyzer is planned to automatically test and compare different CC1101 receiver configurations.

This may include parameters such as:

* RX bandwidth
* data rate
* frequency deviation
* modulation
* synchronization settings
* other relevant CC1101 radio parameters

OpenRF could then compare capture quality across configurations and recommend or automatically select the best-performing RF profile.

### Target workflow

The intended long-term workflow is:

```text
SCAN
  ↓
DETECT
  ↓
ANALYZE
  ↓
OPTIMIZE
  ↓
VERIFY
  ↓
TEST
  ↓
SAVE
  ↓
HOME ASSISTANT
```

The technical complexity should remain inside OpenRF rather than being pushed onto the user.

### Development priority

1. Second CC1101 / dedicated 868 MHz radio
2. Unified RF capture layer
3. Dual-radio RF engine
4. Deep Analyzer foundation
5. Multi-capture comparison
6. Modulation and timing analysis
7. CC1101 Auto Tune
8. Analyze → Save → Home Assistant workflow

LoRa/SX1276, community RF profile sharing, cloud functionality and other radio technologies remain possible future directions, but they are **not the current development priority**.

---

## ESP32-S3 architecture

The ESP32-S3 port uses a FreeRTOS task-based dual-core architecture.

### Core 0 – System

* Wi-Fi
* WebUI
* REST API
* MQTT
* Home Assistant
* OTA
* LittleFS
* configuration and system services

### Core 1 – Radio Engine

* CC1101
* RF RX
* RF TX
* Learn
* RAW capture
* protocol decode
* Kinetic RF
* RF Analyzer

Communication between the two domains is queue-based:

```text
Core 0
  ↓
rfCommandQueue
  ↓
Core 1

Core 1
  ↓
rfEventQueue
  ↓
Core 0
```

This keeps normal network activity from directly blocking the time-sensitive radio engine.

---

## RF Analyzer

On ESP32-S3, the RF Analyzer is **non-exclusive**.

The Analyzer can run at the same time as RX Slots, MQTT, Home Assistant, gateway operation and RF event forwarding.

The ESP8266-era Gateway Mode / Exclusive Analyzer restriction does **not** apply to the ESP32-S3 version.

The Analyzer provides:

* Adjustable RSSI threshold
* Candidate capture and freeze
* Pulse count, duration and RSSI
* Pulse min / average / max
* Pulse-class estimation
* Base-pulse and class-ratio diagnostics
* Alternation analysis
* Same-sign pair count
* Longest same-sign run
* RAW normalization
* Structured Unknown detection
* Similarity and occurrence clustering
* Reject-reason diagnostics
* Known-protocol recognition
* Copyable RAW output for decoder development

The WebUI uses a lightweight live-update path and controlled full refreshes to avoid HTTP backlog while keeping Analyzer data responsive.

---

## PSRAM and memory model

The ESP32-S3 N16R8 build uses:

* **16 MB Flash**
* **8 MB PSRAM**

Time-critical data remains in internal RAM, including ISR RF capture, FreeRTOS task stacks, command and event queues, critical radio state and Wi-Fi / TCP / MQTT runtime.

Large non-ISR work buffers use PSRAM where appropriate, including System RAW scratch, Radio last RAW, Learn RAW buffer and Analyzer preview storage.

PSRAM allocation has internal-RAM fallback and safe failure handling.

The current ISR RF capture limit intentionally remains **600 pulses**. Larger non-ISR work buffers are prepared for up to **2048 pulses**.

---

## Supported hardware

| Hardware       | Status          |
| -------------- | --------------- |
| ESP32-S3 N16R8 | ✅ Supported     |
| CC1101         | ✅ Supported     |
| ESP8266        | Legacy — v1.2.0 |

### Current RF hardware scope

The current ESP32-S3 firmware uses **one active CC1101**.

The following are intentionally outside v2.0.0-beta.1:

* second active CC1101
* simultaneous 433 + 868 MHz operation
* SX1276 / LoRa
* RX Slot → TX
* 2048-pulse ISR capture

See `ESP32_S3_PORT.md` for the port milestones and exact port boundary.

---

## 433 / 868 MHz selection

Select the operating band on the **Settings** page:

* 433 MHz → 433.920 MHz
* 868 MHz → 868.350 MHz

The selected value is stored in LittleFS and applied after restart.

> **Hardware requirement:** Software frequency selection does not retune the antenna or RF matching network. Use a CC1101 module and antenna suitable for the selected band.

---

## Installation

### Requirements

* ESP32-S3 N16R8
* suitable CC1101 module
* correctly tuned antenna
* PlatformIO
* USB data cable

### Build and upload

1. Open the project folder in VS Code with PlatformIO.
2. Verify the `esp32s3` environment in `platformio.ini`.
3. Run **Build**.
4. Run **Upload** for the firmware.
5. Run **Upload Filesystem Image** when the WebUI files in `data/` have changed.

Release binaries are attached to the GitHub release:

* `OpenRF-Platform-v2.0.0-beta.1-ESP32S3-firmware.bin`
* `OpenRF-Platform-v2.0.0-beta.1-ESP32S3-littlefs.bin`

> Uploading the filesystem image replaces LittleFS and may erase Wi-Fi, MQTT, Analyzer and slot configuration. Create a Backup first if configuration must be preserved.

### First start

1. Connect to the `OpenRF-Platform` setup access point.
2. Open `http://192.168.4.1`.
3. Configure Wi-Fi and MQTT.
4. Save the configuration and allow the device to restart.
5. Open the assigned LAN IP address.

---

## RF Learn and TX Slots

RAW Learn records an RF pulse train without requiring protocol knowledge.

```text
Start Learn
  ↓
Transmit original remote signal
  ↓
Inspect preview
  ↓
Accept
  ↓
Save to TX slot
  ↓
Replay
```

TX slots store RAW pulse sequences in LittleFS.

---

## Universal RX Slots

RX Slots store decoded event identity instead of depending on exact RAW equality.

Typical identity fields:

* Protocol
* Device ID or transmitter signature
* Control / button
* Event / code

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

OpenRF Platform publishes TX/RX state and events under the configured MQTT base topic.

Home Assistant Discovery creates supported entities and device triggers automatically.

The ESP32-S3 platform no longer depends on the ESP8266 low-memory Discovery restrictions.

---

## REST API

The WebUI uses the same JSON API available to integrations.

Endpoint groups cover:

* System and health
* Radio state
* Settings
* RF Learn
* TX Slots
* RX Slots
* Analyzer
* OTA
* Backup and Restore

---

## Backup and Restore

Backup includes supported configuration, RF Slots and RX Slots.

Use Backup before uploading a new LittleFS image, migrating configuration, testing a new release or replacing hardware.

---

## Troubleshooting

### Dashboard shows API error

* Confirm the device IP address after restart.
* Confirm that firmware and LittleFS belong to the same release.
* Perform a hard browser refresh (`Ctrl+F5`).
* Re-upload the filesystem image when frontend files were not updated.

### Settings were erased after filesystem upload

Uploading the filesystem image replaces LittleFS. Restore a backup or configure the device again.

### Analyzer data appears delayed

* Confirm the browser is using the current WebUI files.
* Perform a hard refresh.
* Check Core 0 / Core 1 load in the header.
* Check System memory diagnostics.

### 868 MHz selected but range is poor

Use a CC1101 module and antenna designed for 868 MHz. A 433 MHz antenna or module variant may perform poorly even when the CC1101 frequency register is set correctly.

---

## Repository branches

### `esp32-s3`

Default branch and current development platform.

Current public release: **v2.0.0-beta.1**

### `esp8266`

Legacy stable implementation.

Final feature release: **v1.2.0**

The ESP8266 branch may receive critical fixes, but no new feature development is planned.

---

## Repository documentation

* `ESP32_S3_PORT.md` – seven ESP32-S3 port milestones and port boundary
* `ANALYZER_V2_FINAL.md` – RF Analyzer architecture and behaviour
* `CHANGELOG.md` – project version history
* `RELEASE_NOTES_V1.2.0.md` – final ESP8266 release notes
* `docs/` – subsystem and implementation documentation
* `CONTRIBUTING.md` – contribution rules
* `LICENSE` – MIT License

---

## Responsible use

Use OpenRF Platform only with devices you own or are authorized to test.

Follow local radio regulations, permitted frequency bands, power limits and duty-cycle requirements. Do not use the project to interfere with other radio users or bypass security systems.

---

## License

OpenRF Platform is released under the MIT License. See [LICENSE](LICENSE).

---

## Credits

Created and maintained by **Kocsis Krisztián**.

Developed by Kocsis Krisztián with implementation assistance, architecture discussions and documentation support from **ChatGPT (OpenAI)**.

⭐ If OpenRF Platform is useful to you, consider starring the project on GitHub.

---

<!-- OPENRF_STATS_START -->
## OpenRF Platform Statistics

- Repository views: **912**
- Repository clones: **85**
- Tracking since: **2026-08-07**

<!-- OPENRF_STATS_END -->

---

## ☕ Support OpenRF Platform

OpenRF Platform is free and open source.

If you find the project useful and would like to support its continued development, hardware testing and future features, you can buy me a coffee:

[☕ Buy me a coffee](https://buymeacoffee.com/krissz55555)

Thank you for supporting OpenRF Platform!
