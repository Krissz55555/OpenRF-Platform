# OpenRF Platform

<p align="center">
  <img src="assets/openrf-platform-logo.svg" width="320" alt="OpenRF Platform Logo">
</p>

<h1 align="center">OpenRF Platform</h1>

<p align="center">
<strong>Open Source RF Platform for ESP8266 featuring CC1101, MQTT, REST API and native Home Assistant integration.</strong>
</p>

<p align="center">
RF Gateway • RF Analyzer • Home Assistant • MQTT • REST API • OTA
</p>

---

**Current stable release: v1.1.0**

OpenRF Platform is a modern, modular RF platform built for ESP8266 devices equipped with a CC1101 transceiver.

Rather than focusing on supporting a limited number of remote controls, OpenRF Platform provides a stable and extensible foundation for RF communication. It combines RF transmission, protocol-aware reception, RF analysis and Home Assistant integration into a single maintainable firmware.

The project is designed around one simple principle:

> **Keep the platform stable while allowing RF protocols to evolve independently.**

---

## RF Analyzer

The RF Analyzer is one of the key technologies behind OpenRF Platform.

Its primary purpose is not simply to display RF signals, but to provide a structured and repeatable workflow for understanding unknown radio transmissions before protocol support is implemented.

Instead of relying on assumptions or device-specific hacks, every new RF protocol begins with signal analysis. The Analyzer provides the tools required to capture, compare, validate and classify RF transmissions, allowing new protocol support to be developed using measurable and reproducible data.

This approach separates protocol development from the core platform, keeping the firmware stable while enabling continuous expansion of supported devices.

As OpenRF Platform evolves, the Analyzer will remain the foundation for integrating new remote controls, wireless switches, sensors, weather stations and other RF devices through a consistent engineering-driven development process.

---

# Why OpenRF Platform?

Most RF gateway projects are built around a fixed collection of supported RF devices.

OpenRF Platform follows a different philosophy.

Instead of embedding protocol-specific logic throughout the firmware, the platform separates the RF infrastructure from protocol implementations. New RF protocols can be added as standalone decoder modules without modifying the gateway, MQTT layer, REST API or Home Assistant integration.

This architecture keeps the project clean, maintainable and scalable for future development.

---

# Why ESP8266?

ESP8266 is no longer the newest microcontroller, but millions of NodeMCU boards are still perfectly usable.

OpenRF Platform intentionally gives these devices a second life.

For RF gateway applications, ESP8266 still provides excellent performance while remaining inexpensive, reliable and widely available.

---

# Features

## RF Gateway

- Learn unknown RF signals
- Replay RAW transmissions
- 30 persistent RAW TX slots
- Protocol-independent signal storage

## RF Receiver

- Native protocol decoding
- Universal RX Slots
- Stable decoded RF events
- MQTT publishing
- Home Assistant event generation

## RF Analyzer v2

Integrated RF diagnostics for known and unknown 433 MHz signals:

- Adjustable Analyzer-only RSSI threshold
- Candidate capture before Analyzer filtering
- RSSI, pulse count, duration and reject-reason diagnostics
- Structured unknown signal detection
- Similarity and occurrence analysis
- Original RAW pulse export
- Normalized RAW pulse analysis
- Alternation ratio, same-sign pair and longest-run statistics
- Visible Developer Mode with advanced Analyzer controls
- Persistent Analyzer settings stored in LittleFS

Standard Mode retains the clean v1.0.0 Analyzer layout and original filtering behaviour. Developer Mode enables the Analyzer v2 candidate pipeline, advanced thresholds, normalization metrics and detailed processing statistics. Analyzer settings do not alter RX Slots, RF Learn, MQTT or Home Assistant processing.

## Home Assistant

- Native MQTT Discovery
- Binary Sensor support
- Automatic entity creation
- Automation-friendly RF events

## Web Interface

- Dashboard
- RF Learn
- TX Slots
- RX Slots
- RF Analyzer
- Settings
- System
- OTA Update

## System Features

- OTA firmware updates
- Backup / Restore
- LittleFS configuration storage
- REST API
- Access Point setup
- Wi-Fi Station mode

---

# Supported Hardware

| Hardware | Status |
|----------|--------|
| ESP8266 NodeMCU V2 | ✅ Supported |
| ESP-12E | ✅ Supported |
| CC1101 | ✅ Supported |
| ESP32 | 🚧 Planned |
| SX127x | 🚧 Planned |
| SX126x | 🚧 Planned |

---

# Reference Wiring

| CC1101 | ESP8266 |
|---------|----------|
| VCC | 3V3 |
| GND | GND |
| SCK | GPIO14 (D5) |
| MISO | GPIO12 (D6) |
| MOSI | GPIO13 (D7) |
| CSN | GPIO15 (D8) |
| GDO0 | GPIO4 (D2) |
| GDO2 | Not connected |

> **Note:** Only the **GDO0** interrupt pin is used. GDO2 is intentionally left unconnected.

---

# Architecture

```text
               RF Signal
                   │
                   ▼
             Noise Gate
                   │
                   ▼
          Protocol Decoder
                   │
                   ▼
          DecodedRFEvent
                   │
                   ▼
          Universal RX Slot
              │           │
              ▼           ▼
            MQTT   Home Assistant
```

Unlike traditional RAW matching systems, OpenRF Platform performs RF reception using decoded protocol information, resulting in more reliable automation events and easier protocol expansion.

---

# RAW Transmission

```text
RF Learn
    │
    ▼
Preview
    │
    ▼
Save to TX Slot
    │
    ▼
Replay
```

RAW Learn stores the complete pulse sequence exactly as received.

No protocol knowledge is required to save or replay a transmission.

> Some receivers marketed as rolling-code systems may still respond to RAW replay depending on their implementation. Rolling-code compatibility is **not guaranteed**.

---

# Supported Protocols

Current native decoder support includes:

- EV1527
- HS1527
- Princeton
- PT2262 / PT2272 style protocols

The decoder framework is modular and designed for future protocol extensions.

---

# REST API

The REST API is considered stable and version-independent.

Current endpoint groups include:

- System
- Radio
- RF Learn
- TX Slots
- RX Slots
- RF Analyzer
- Settings
- OTA
- Backup / Restore

The Web Interface uses the same JSON API internally.

---

# MQTT & Home Assistant

OpenRF Platform integrates directly with Home Assistant using MQTT Discovery.

Features include:

- Automatic entity creation
- Binary Sensor events
- TX Slot control
- Automation triggers
- Repeat lockout protection

---

# Project History

| Version | Milestone |
|----------|-----------|
| v0.2.x | RAW Learn & Replay |
| v0.3.x | MQTT & Home Assistant Discovery |
| v0.4.x | Stability & Memory Improvements |
| v0.5.x | OTA & Backup / Restore |
| v0.6.x | RX Slots & WebUI |
| v0.7.x | Native Decoder Framework & Universal Decoder API |
| v0.8.x | RF Analyzer & Optimized Decoder Core |
| **v1.0.0** | **First Stable Release as OpenRF Platform** |

Detailed development history is available in the **docs/** directory.

---

# Design Philosophy

OpenRF Platform is **not** intended to support every RF protocol.

Its primary goal is to provide a stable, well-documented and modular RF platform.

New RF protocols should be implemented as independent decoder modules without modifying the gateway, REST API, MQTT infrastructure or Home Assistant integration.

Keeping the platform stable while allowing protocol support to grow has been the project's guiding principle from the beginning.

---

# Roadmap

Planned future development includes:

- Native ESP32 support
- SX127x support
- SX126x support
- Additional protocol modules
- Community decoder contributions
- Enhanced RF Analyzer
- Extended REST API

---

# Documentation

Additional technical documentation is available in the **docs/** directory, including architecture notes, development history and implementation details.

---

# Responsible Use

OpenRF Platform is intended for legitimate RF experimentation, interoperability and home automation.

This project is designed to help users analyze, learn from and control RF devices that they own or are explicitly authorized to test.

Please use OpenRF Platform responsibly.

Do **not** use this software to:

- Control devices that you do not own or do not have permission to operate.
- Bypass or defeat security mechanisms.
- Interfere with radio communications or other users.
- Operate outside the radio regulations applicable in your country.

RAW signal learning and replay are provided for compatibility testing, research and interoperability with supported devices. Some rolling-code receivers may respond to RAW replay depending on their implementation, but OpenRF Platform is **not** intended to circumvent rolling-code security systems.

The authors assume no responsibility for misuse of this software. Users are solely responsible for ensuring compliance with all applicable laws, regulations and radio frequency requirements.

---

# License

This project is released under the **MIT License**.

See the [LICENSE](LICENSE) file for details.

---

# Credits

Created and maintained by **Kocsis Krisztián**

Architecture, design discussions and documentation support by **ChatGPT (OpenAI)**

---

<p align="center">
⭐ If you find OpenRF Platform useful, consider giving the project a star on GitHub.
</p>