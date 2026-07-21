# OpenRF Platform v0.7.0 – Protocol Decoder Engine

RX automation no longer uses fuzzy RAW matching.

## Rules

- TX remains RAW Learn + RAW Replay.
- RX triggers require an exact decoded protocol, symbol count and code.
- Unknown or ambiguous signals are rejected.
- Duplicate RX codes cannot be saved twice.

## Initial protocol families

- EV1527 / HS1527 / Princeton-style binary PWM/OOK
- PT2262-style tri-state OOK

Rolling-code, encrypted and Manchester-coded remotes are not supported in this first decoder release.
