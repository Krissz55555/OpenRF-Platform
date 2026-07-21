# OpenRF Platform v0.6.0 – RX Slot Engine

- 10 independent receiver slots for RF remote buttons.
- Learn, relearn, rename, enable/disable and delete from WebUI.
- Tolerant RAW pulse matching with duration normalization and an 82% threshold.
- 700 ms per-slot lockout prevents a single repeated RF burst from creating multiple Home Assistant actions.
- MQTT event: `openrf/<device>/rxslot/<n>/event`.
- Home Assistant MQTT device automation trigger for each enabled RX slot.
- RX slots are included in `.orfbackup` export/restore.
