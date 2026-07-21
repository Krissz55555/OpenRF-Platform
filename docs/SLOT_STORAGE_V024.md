# OpenRF Platform v0.2.4 - Slot Storage

- 30 persistent LittleFS RF slots
- Versioned binary slot format
- FNV-1a fingerprint and load-time integrity check
- Save current validated Learn preview
- Send saved slot with configured Replay Count
- Rename and delete slots
- RF Slots WebUI and REST API

The stable v0.2.3a receiver, validator and direct TX timing remain unchanged except that the proven TX routine is exposed through `Radio.sendRaw()` for stored signals.
