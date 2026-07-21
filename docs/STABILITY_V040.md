# OpenRF Platform v0.4.0 - Performance & Stability

## Normal monitoring filter

Normal RX monitoring rejects a candidate only when both conditions are true:

- fewer than 60 pulses;
- shorter than 80 ms.

This suppresses common short OOK background bursts without preventing Learn mode from capturing them. Learn mode continues to use the broad structural validator and its measured RSSI-over-noise-floor check.

## Runtime diagnostics

`GET /api/status` now includes:

- `free_heap`
- `max_free_block`
- `heap_fragmentation_percent`
- `uptime_seconds`
- `reset_reason`

`GET /api/radio/debug` also includes the background-filter counter and active monitor thresholds.

## Long-duration test

Keep the bridge powered for at least 24 hours and verify:

1. MQTT remains connected or reconnects automatically.
2. WebUI remains responsive.
3. Free heap does not continuously decrease.
4. Heap fragmentation remains reasonably stable.
5. Learned slots still transmit reliably.
6. Learn, delete and relearn continue to work from Home Assistant.
