# ESP8266 Stability Test – Kinetic / MQTT / Home Assistant

This test build addresses the heap corruption observed after Learn/slot save and Home Assistant discovery.

## Changes

1. Home Assistant MQTT discovery is no longer published as one large blocking operation.
   - Discovery is queued.
   - It starts 3 seconds after the request.
   - Only one discovery item is published per loop step.
   - Steps are separated by 75 ms.
   - Publishing pauses automatically if free heap or the largest contiguous heap block is too small.

2. RAW buffers were reduced from 2000 to 600 pulses for the ESP8266 profile.
   - The receiver already rejects accepted frames above 500 pulses.
   - 600 leaves a safety margin.
   - Four RAW buffers are affected: ISR capture, last frame, Learn frame and shared scratch.
   - Estimated RAM saving: about 11.2 kB.

3. Existing calls to `mqttPublishDiscovery()` now schedule discovery instead of running it inside web, MQTT or Learn callbacks.

## What remains unchanged

- NVKP01 recognition and Kinetic PRESS event
- RX Slot matching
- RAW/TX Learn behavior
- MQTT topics and Home Assistant entity structure
- EV1527/Princeton and PT2262 decoding
- LittleFS slot formats

## Test sequence

1. Build and upload firmware and LittleFS only if required.
2. Record the boot line `Free heap after startup`.
3. Wait for `Home Assistant discovery published gradually`.
4. Learn an NVKP01 RX Slot.
5. Save, rename and delete both TX and RX slots several times.
6. Keep Serial Monitor running for several hours.
7. Test at least 100 Home Assistant lamp toggles.
8. Watch the periodic health line for heap, largest block and fragmentation.

A reset with a decoded allocator/LittleFS/lwIP stack must still be treated as a failed stability test.
