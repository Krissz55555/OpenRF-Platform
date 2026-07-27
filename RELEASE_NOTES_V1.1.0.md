# OpenRF Platform v1.1.0 — Analyzer v2 Final

OpenRF Platform v1.1.0 keeps the stable v1.0.0 Analyzer experience as the default while adding an explicit Developer Mode for advanced RF diagnostics.

## Standard mode

- Clean v1.0.0 Analyzer layout
- Original v1.0.0 Analyzer filtering behaviour
- Decoded and accepted RF events remain easy to inspect
- No candidate diagnostics or advanced controls clutter the page

## Developer Mode

- Adjustable Analyzer RSSI threshold
- Minimum pulse count and duration controls
- Similarity and occurrence controls
- Alternation tolerance
- Structured rejected-signal detection
- Last Candidate diagnostics
- Normalized RAW pulse analysis
- Freeze Last Candidate

Developer Mode affects only Analyzer diagnostics. RX slots, RF Learn, MQTT and Home Assistant processing remain independent.
