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