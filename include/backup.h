#pragma once

#include <Arduino.h>
#include <WiFi.h>

size_t backupCalculateSize(uint16_t& fileCount);
bool backupStreamToClient(WiFiClient& client, String& error);
bool backupRestoreFromFile(const char* uploadPath, String& error);
