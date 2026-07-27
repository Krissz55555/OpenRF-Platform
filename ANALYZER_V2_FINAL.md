# OpenRF Platform — Analyzer v2 Final

Analyzer v2 extends the stable v1.0.0 RF Analyzer with an explicit advanced diagnostics mode.

## Standard Mode

Standard Mode intentionally retains the clean v1.0.0 Analyzer page and its original filtering behaviour:

- Latest RF event status
- Frequency, RSSI, protocol and encoding
- Symbol, pulse and frame metrics
- Analyzer counters
- Signal report and RAW export

Candidate capture, rejected-signal clustering and Analyzer-only RSSI filtering are inactive in Standard Mode.

## Developer Mode

- Adjustable Analyzer-only RSSI threshold
- Minimum pulse count and duration
- Similarity and required occurrences
- Alternation tolerance
- Structured rejected-signal detection
- Last Candidate diagnostics
- Alternation ratio, same-sign pairs and longest run
- Normalized pulse count and normalized RAW
- Freeze Last Candidate

Developer Mode is clearly visible with a yellow ON/OFF control. Its state and settings are stored in LittleFS. Analyzer controls remain isolated from RX Slots, RF Learn, MQTT and Home Assistant processing.
