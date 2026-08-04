# NVKP01 Kinetic integration test

This test build enables the first actionable Kinetic protocol.

## Behaviour

- A structurally valid NVKP01 telegram decodes as `PRESS`.
- Device ID: `nvkp01`
- Control ID: `button`
- Both mechanical phases are intentionally normalized to the same event.
- RX Slot lockout suppresses the closely spaced duplicate telegram during a normal click.
- PRESS/RELEASE separation is deliberately deferred.

## Learn and RX Slots

Use **RX Slots → Capture button**. The learned slot should show:

- Protocol: `NVKP01 Kinetic`
- Device ID: `nvkp01`
- Command: `button`
- Code/Event: `PRESS`

The classic RAW/TX Learn system remains unchanged. NVKP01 should be learned as an RX event, not as a replayable TX slot.

## MQTT

A matched RX slot publishes to:

`openrf/<hostname>/rxslot/<slot>/event`

Example payload:

```json
{
  "event": "pressed",
  "action": "PRESS",
  "protocol": "NVKP01 Kinetic",
  "device_id": "nvkp01",
  "command": "button",
  "code": "PRESS"
}
```

The legacy `event: pressed` field is preserved for Home Assistant compatibility.

## Home Assistant

After the RX slot is learned, MQTT Discovery creates:

- a device automation trigger (`button_short_press`), and
- a binary sensor that turns on briefly for each matched click.

## Test checklist

1. Build and upload firmware.
2. Confirm Analyzer shows `NVKP01 Kinetic` and `Kinetic marker train`.
3. Start RX Slot learning and click the kinetic button once.
4. Confirm the slot saves as `NVKP01 Kinetic / nvkp01 / button / PRESS`.
5. Click repeatedly and verify one RX Slot event per normal click.
6. Verify MQTT payload and Home Assistant trigger.
7. Test slow/long holds; report if release creates a second event after the 700 ms lockout.

### NVKP01 false-trigger protection

Actionable NVKP01 RX Slot, MQTT and Home Assistant events now require two short, structurally valid Kinetic captures within one second. The short-frame envelope (28–70 pulses, 18–60 ms) excludes long RF remote trains, while the second capture corresponds to the mechanical press/release cycle. Analyzer recognition remains intentionally broader for diagnostics.
