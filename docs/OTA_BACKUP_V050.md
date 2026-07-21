# OpenRF Platform v0.5.0 – OTA + Backup

## Firmware OTA

The System page accepts the PlatformIO `firmware.bin`. The updater writes only the firmware partition (`U_FLASH`), so LittleFS remains unchanged. This preserves `config.json` and all `/slots/slotN.bin` files.

Do not upload a LittleFS image through this page. PlatformIO **Upload Filesystem Image** still replaces the filesystem.

## Backup format

The `.orfbackup` file is a versioned binary container containing:

- `/config.json`
- every existing `/slots/slotN.bin` file

The configuration includes saved WiFi and MQTT credentials. Keep backups private. Restore validates the container and extracts all records to temporary files before replacing live data.

## Web endpoints

- `GET /api/system/backup`
- `POST /api/system/restore` (multipart upload)
- `POST /api/system/ota` (multipart upload)
