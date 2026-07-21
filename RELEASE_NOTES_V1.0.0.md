# OpenRF Platform v1.0.0 — First Stable Release

This release promotes the tested v0.8.0 feature set to the first stable OpenRF Platform release. No new RF protocol experiment was added.

## Validation checklist

- [ ] PlatformIO firmware build succeeds without errors
- [ ] LittleFS image uploads and WebUI loads
- [ ] First-start AP is `OpenRF-Platform` at `192.168.4.1`
- [ ] Existing configuration remains readable after upgrade
- [ ] RAW Learn captures and previews a signal
- [ ] RAW signal saves to and replays from multiple TX slots
- [ ] Existing fixed-code RX slots still load
- [ ] New RX slot captures a supported protocol and fires once per press
- [ ] RF Analyzer stays quiet without a real accepted signal
- [ ] MQTT reconnect and Home Assistant Discovery work
- [ ] OTA update completes and restarts cleanly
- [ ] Backup downloads and restore completes
- [ ] Free heap and fragmentation remain stable during extended operation

## Upgrade note

The compiled firmware does not automatically rename a previously saved hostname. Existing installations may continue using `OpenRF-Bridge` until the hostname is changed in Settings. Fresh installations default to `OpenRF-Platform`. This avoids silently breaking DHCP, mDNS and MQTT naming on upgrades.

### WebUI logo fix

The embedded web server now explicitly serves `/logo.svg` from LittleFS, so the header, About page and favicon can load the bundled OpenRF Platform logo.
