# OpenRF Platform Radio Core v1

Firmware version: `0.2.0-alpha`

## Reference wiring

| CC1101 | NodeMCU V2 |
|---|---|
| VCC | 3V3 |
| GND | GND |
| SCK | D5 / GPIO14 |
| MISO | D6 / GPIO12 |
| MOSI | D7 / GPIO13 |
| CSN | D8 / GPIO15 |
| GDO0 | D2 / GPIO4 |
| GDO2 | D1 / GPIO5 |

The CC1101 must be powered from **3.3 V**, never 5 V.

## New API

`GET /api/radio`

Example response:

```json
{
  "initialized": true,
  "chip": "CC1101",
  "frequency_mhz": 433.92,
  "mode": "RX",
  "receiving": true,
  "rssi_dbm": -72.5,
  "last_error": 0,
  "modulation": "OOK"
}
```

## Scope

This milestone initializes the CC1101, enables OOK mode, starts RX, reads RSSI and exposes live radio status through the REST API and dashboard. RAW Learn and Replay are intentionally not included yet.
