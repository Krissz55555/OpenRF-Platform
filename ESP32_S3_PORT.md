# OpenRF Platform – ESP32-S3 Port

This document records the essential milestones of the ESP8266 → ESP32-S3 port.
It is intentionally not a development diary: intermediate experiments, FIX builds
and temporary implementation notes are omitted.

The port started from **OpenRF Platform v1.2.0 Final for ESP8266**. Its purpose was
to preserve the complete v1.2.0 feature set on ESP32-S3 first, then remove
limitations that existed only because of the ESP8266 hardware.

## Step 1 – ESP32-S3 base port

The ESP8266-specific implementation was moved to ESP32-S3 while keeping the
existing OpenRF Platform behaviour and feature set.

Ported and retained:

- Wi-Fi STA and AP
- WebUI and REST API
- LittleFS
- OTA
- Backup / Restore
- CC1101
- RF Learn and RAW TX
- RF Slots and RX Slots
- RF Analyzer
- MQTT
- Home Assistant Discovery
- RF event forwarding
- NVKP01 / Kinetic RF

Target hardware: **ESP32-S3 N16R8** with 16 MB Flash and 8 MB PSRAM.

## Step 2 – ESP8266 limitations removed

Legacy throttling and protection mechanisms that were required by the ESP8266
were reviewed and removed where they were no longer necessary.

This included the old Analyzer low-heap restrictions, Home Assistant heap
thresholds, Analyzer exclusive behaviour and unnecessary event throttling.

The result is that Analyzer, RX Slots, MQTT, Home Assistant and normal gateway
operation can run concurrently.

## Step 3 – Dual-core architecture

The firmware was separated into two FreeRTOS task domains:

**Core 0 – System**

- Wi-Fi
- WebUI / REST
- MQTT
- Home Assistant
- OTA
- LittleFS
- configuration and system services

**Core 1 – Radio Engine**

- CC1101
- RF RX / TX
- Learn
- RAW capture
- protocol decode
- Kinetic RF
- Analyzer

This prevents normal network and system activity from directly blocking the
time-sensitive radio engine.

## Step 4 – Queue-based core communication

Communication between the two task domains was separated through FreeRTOS
queues.

System → Radio commands use `rfCommandQueue`.

Radio → System events use `rfEventQueue`.

MQTT, Home Assistant, REST and WebUI therefore no longer need to perform normal
RF operations directly inside the Radio Engine.

## Step 5 – Analyzer modernization

The Analyzer was converted from the ESP8266-era exclusive model to concurrent
operation.

A lightweight live-status path and controlled full refresh were added so the
WebUI can follow current RF activity without continuously requesting the full
Analyzer state.

Candidate updates are debounced and stale intermediate UI states are not
replayed. Analyzer operation can coexist with RX Slots, MQTT, Home Assistant and
gateway traffic.

## Step 6 – N16R8 and PSRAM memory architecture

The ESP32-S3 N16R8 configuration was corrected so the firmware uses the full
hardware:

- 16 MB Flash
- 8 MB PSRAM

Time-critical data remains in internal RAM, including ISR RF capture, FreeRTOS
task stacks, queues and critical radio state.

Large non-ISR working areas use PSRAM where appropriate, including RAW/Learn and
Analyzer preview storage. PSRAM allocations have internal-RAM fallback and safe
failure handling.

The ISR capture limit intentionally remains **600 pulses**. Larger non-ISR
working buffers are prepared for up to **2048 pulses**; this does not change the
current RF capture limit.

## Step 7 – Final stabilization

The port was completed with the remaining stability and diagnostic work:

- adjustable RX Slot Learn RSSI filtering
- live Core 0 / Core 1 and memory diagnostics
- Flash / PSRAM / internal heap diagnostics
- Analyzer rejected-candidate safety fix
- complete RX Slot Backup / Restore handling
- additional FreeRTOS task stack headroom for the larger ESP32-S3 queue messages
- full functional regression testing of the existing v1.2.0 feature set

At the end of this step, the original OpenRF Platform v1.2.0 functionality is
available on the ESP32-S3 port without introducing the planned next-generation
RF features.

## Port boundary

The following are deliberately **not part of this port**:

- active second CC1101
- simultaneous 433 + 868 MHz operation
- SX1276 / LoRa
- RX Slot → TX
- new RF protocols
- weather-station support
- 2048-pulse ISR capture

These belong to the next development phase after the ESP32-S3 port is frozen
and has completed its stability test.
