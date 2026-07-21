# OpenRF Platform v0.2.2-alpha — RAW Receiver

This release adds continuous CC1101 OOK RAW pulse capture.

## Receiver parameters

- GDO0 interrupt: CHANGE
- Noise filter: 150 us
- Frame gap: 25,000 us
- Minimum frame: 20 pulses
- Maximum frame: 2,000 pulses

## REST API

- `GET /api/radio`
- `GET /api/radio/raw`

The RAW endpoint returns the most recently received frame. Storage, learning slots,
and replay are intentionally not enabled in this phase.
