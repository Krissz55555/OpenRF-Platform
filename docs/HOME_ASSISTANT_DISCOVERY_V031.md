# Home Assistant Discovery v0.3.1

When MQTT and Home Assistant Auto Discovery are enabled, OpenRF Platform publishes retained discovery configuration after connecting to the broker.

Entities:
- One MQTT button for each saved RF slot.
- Diagnostic bridge status sensor.
- Last RF pulse count sensor.
- Last RF RSSI sensor.

Empty or deleted slots publish an empty retained discovery payload so stale buttons are removed from Home Assistant. Slot save, rename, and delete operations refresh discovery automatically.
