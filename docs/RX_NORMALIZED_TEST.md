# RX Normalized Capture Test

This is an isolated test build based on OpenRF Platform v1.1.0.

Changes:
- 5 ms RSSI refresh for more reliable frame peak RSSI.
- Sub-150 us glitches are ignored without moving the accepted edge reference.
- Consecutive same-sign RAW pulses are merged in the capture layer.
- Capture termination diagnostics are exposed through the debug APIs.
- The candidate freeze label is renamed to "Pause candidate updates".

This folder intentionally contains no .git directory, so it cannot be committed or pushed as the original repository by accident.
