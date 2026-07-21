# Universal Decoder API

The decoder layer normalizes supported native RF protocols into a common `DecodedRFEvent`.

Supported baseline protocols:
- EV1527 / HS1527 / Princeton-compatible binary PWM
- PT2262 / PT2272-compatible tri-state PWM

RX slots match protocol and decoded code through the same generic interface. RAW Learn and RAW Replay remain independent and unchanged.
