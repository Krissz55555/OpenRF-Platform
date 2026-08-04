# NVKP01 reverse-engineering notes

## Confirmed

- Carrier frequency: 433.920 MHz
- Clean captures alternate polarity correctly.
- Representative leading sequence observed repeatedly:
  - LOW approximately 1485-1513 us
  - HIGH approximately 653-721 us
  - LOW approximately 5705-5753 us
  - HIGH approximately 653-684 us

## Working hypotheses

- The first four pulses are a leader or synchronization sequence.
- The remaining field may contain variable-length or repeated symbols.
- Current samples have different pulse counts and may contain partial frames.

## Do not assume yet

- PWM, pulse-distance or Manchester encoding
- fixed bit count
- button field location
- checksum
- rolling or fixed code
- release/hold/double-click events

## Next measurement requirement

Collect multiple full frames from one button with near-identical pulse count and
duration, then repeat with at least one different button. Compare aligned pulse
pairs before implementing protocol recognition.

## Capture-position-independent recognition

The recognition matcher scans the complete alternating RAW capture instead of
requiring the NVKP marker at pulse zero. A capture is recognized when it contains
a complete marker/sync leader, at least three marker pairs, or two marker pairs
plus a measured 5.0-6.3 ms LOW sync pulse. Recognition remains non-actionable.

