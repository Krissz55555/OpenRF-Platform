# RF Analyzer v0.7.1

The Analyzer is a passive observer attached to the finalized RAW receive path. It processes every RF candidate after capture validation and exposes the latest result through `GET /api/analyzer`.

It does not modify TX slots, RX slots, MQTT, Home Assistant discovery, learning, replay or existing REST endpoints.

## Test checklist

1. Upload firmware and LittleFS.
2. Open **RF Analyzer**.
3. Press known and unknown 433 MHz remote buttons.
4. Verify sequence, RSSI, pulse count and pulse classes update.
5. Verify EV1527/Princeton or PT2262 results show protocol, code and quality when decoded.
6. Verify unknown/rejected frames display their reason without creating an RX event.
7. Re-test RAW Learn, TX replay, RX Slots, MQTT and OTA for regression.


## alpha3 unknown-protocol diagnostics
- Signed RAW pulse preview (up to 192 pulses)
- Minimum, average and maximum pulse widths
- Estimated shortest/base pulse
- Pulse-class ratio
- Copyable signal report for decoder development
