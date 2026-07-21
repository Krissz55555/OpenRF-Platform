# Receiver Tuning v0.2.3a

This release filters idle OOK noise before Learn preview and adds the first RAM-only RAW replay test.

Validation defaults:
- RX bandwidth: 325 kHz
- Capture noise pulse minimum: 150 us
- Frame gap: 25 ms
- Valid frame: 30–500 pulses
- Valid duration: 10–400 ms
- Learn RSSI: at least 6 dB above measured idle noise floor

Test Send uses the Replay Count configured in Settings and returns the CC1101 to RAW RX afterwards.
