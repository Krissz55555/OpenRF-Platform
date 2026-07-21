# OpenRF Platform v0.6.0b – UX Polish

This release freezes the primary WebUI navigation and keeps existing REST, MQTT, backup and slot formats compatible.

## RX capture workflow

The RX Slots page automatically polls the receiver after Capture is pressed. It displays progress and refreshes the saved slot without requiring the user to press Refresh.

## Home Assistant

Each enabled RX slot publishes both an MQTT device automation trigger and a one-second binary sensor pulse. Discovery publication is retried after MQTT reconnects.
