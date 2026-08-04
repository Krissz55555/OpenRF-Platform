# OpenRF Protocol Manager

Status: development integration

The Protocol Manager is the single decoder entry point used by `universalDecode()`.

Decoder order:

1. Existing stable classic decoder (`EV1527/Princeton`, `PT2262`)
2. Registered Kinetic decoders

A result can be:

- **unrecognized**: no decoder matched
- **recognized, non-actionable**: protocol structure matched but payload/event decoding is pending
- **actionable**: a complete decoded event safe for RX Slots, MQTT and Home Assistant

The NVKP01 integration currently uses the second state. This allows Analyzer to show
`NVKP01 Kinetic` without changing existing RX learning or automation behaviour.
