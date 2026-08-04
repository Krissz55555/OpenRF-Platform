# NVKP01 Kinetic decoder

**Status:** Framework scaffold / reverse engineering  
**Category:** Kinetic protocol  
**Frequency:** 433.920 MHz

This is the first reference decoder for the OpenRF Kinetic Protocol Framework.
Its purpose is to translate protocol-specific RF traffic into high-level,
protocol-independent events.

## Intended data flow

```text
RAW pulses
  -> NVKP01 decoder
  -> KineticEvent
  -> Universal RX
  -> MQTT
  -> Home Assistant
```

The decoder must remain independent from MQTT, Home Assistant, WebUI, storage
and RX Slots. It only validates and decodes one received frame.

## Current state

- RX capture alternation validated
- Same-sign pulse pairs eliminated in the test capture path
- Initial RAW samples collected
- Stable leading timing candidate observed
- Decoder interface created
- Leader recognition implemented and registered in the Protocol Manager
- Analyzer can display recognition-only matches
- Bit and event decoding not implemented yet

`matches()` recognizes the measured NVKP01 leader. `decode()` deliberately remains
non-actionable until the payload is confirmed. Recognition is visible only in Analyzer
diagnostics; RX learning, RX Slots, MQTT and Home Assistant continue to ignore it.

## Initial target events

- `PRESS`
- future events only when supported by measured RF data

`RELEASE`, `LONG_PRESS` and `DOUBLE_PRESS` must not be inferred unless the
radio traffic or a higher-level event state machine can establish them
reliably.
