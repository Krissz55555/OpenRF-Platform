# RX capture stability update

This update changes only the low-level RAW receive capture and Developer Mode diagnostics.

## Changes

- RSSI sampling interval reduced from 20 ms to 5 ms so short OOK transmissions are more likely to retain their real peak RSSI.
- Sub-150 us glitch edges no longer move the accepted-edge timestamp or signal level. This prevents one real pulse from being fragmented and reduces artificial same-sign pulse runs.
- RAW capture now finalizes immediately if the pulse buffer becomes full instead of silently dropping subsequent pulses.
- Added diagnostic counters to the Analyzer JSON response:
  - `ignored_glitch_edges`
  - `gap_finalized_frames`
  - `timeout_finalized_frames`
  - `buffer_full_frames`
- The UI label `Freeze Last Candidate` was renamed to `Pause candidate updates` to match its existing behavior.

## Unchanged behavior

- RF Learn thresholds and validation rules
- RX Slot matching
- MQTT and Home Assistant processing
- Frame gap value (25 ms)
- Analyzer classification thresholds
- Stored configuration format

A multi-frame splitter was deliberately not added to the capture core in this update. Splitting repeated protocol frames belongs above RAW capture; adding it here could change RF Learn and RX Slot behavior for existing users.
