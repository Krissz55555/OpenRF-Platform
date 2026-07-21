#include <Arduino.h>
#include <LittleFS.h>
#include <ESP8266WiFi.h>

#include "backup.h"
#include "storage.h"

namespace {
constexpr char BACKUP_MAGIC[8] = {'O','R','F','B','K','P','1','\0'};
constexpr uint16_t BACKUP_VERSION = 1;
constexpr uint32_t MAX_BACKUP_FILE_SIZE = 256UL * 1024UL;

#pragma pack(push, 1)
struct BackupHeader {
  char magic[8];
  uint16_t version;
  uint16_t fileCount;
};

struct RecordHeader {
  uint16_t pathLength;
  uint32_t dataLength;
};
#pragma pack(pop)

String slotPath(uint8_t slot) {
  return "/slots/slot" + String(slot) + ".bin";
}
String rxSlotPath(uint8_t slot) { return "/rxslot" + String(slot) + ".bin"; }


String restorePathFor(const String& finalPath) {
  if (finalPath == "/config.json") return "/config.restore";
  if (finalPath.startsWith("/rxslot") && finalPath.endsWith(".bin")) return finalPath + ".restore";
  if (finalPath.startsWith("/slots/slot") && finalPath.endsWith(".bin")) {
    String name = finalPath.substring(String("/slots/").length());
    name.replace(".bin", ".restore");
    return "/slots/" + name;
  }
  return "";
}

bool allowedPath(const String& path) {
  if (path == "/config.json") return true;
  if (path.startsWith("/rxslot") && path.endsWith(".bin")) {
    const int n = path.substring(7, path.length() - 4).toInt();
    return n >= 1 && n <= 10 && path == rxSlotPath(static_cast<uint8_t>(n));
  }
  if (!path.startsWith("/slots/slot") || !path.endsWith(".bin")) return false;
  const int begin = String("/slots/slot").length();
  const int end = path.length() - 4;
  if (end <= begin) return false;
  const int slot = path.substring(begin, end).toInt();
  return slot >= 1 && slot <= OPENRF_SLOT_COUNT && path == slotPath(static_cast<uint8_t>(slot));
}

bool readExact(File& file, uint8_t* destination, size_t length) {
  size_t total = 0;
  while (total < length) {
    const size_t got = file.read(destination + total, length - total);
    if (got == 0) return false;
    total += got;
    yield();
  }
  return true;
}

bool writeExact(File& file, const uint8_t* source, size_t length) {
  size_t total = 0;
  while (total < length) {
    const size_t written = file.write(source + total, length - total);
    if (written == 0) return false;
    total += written;
    yield();
  }
  return true;
}

bool streamExact(WiFiClient& client, const uint8_t* source, size_t length) {
  size_t total = 0;
  while (total < length) {
    const size_t written = client.write(source + total, length - total);
    if (written == 0) return false;
    total += written;
    yield();
  }
  return true;
}

bool includePath(const String& path) {
  return LittleFS.exists(path);
}

size_t recordSize(const String& path) {
  File file = LittleFS.open(path, "r");
  if (!file) return 0;
  const size_t size = sizeof(RecordHeader) + path.length() + file.size();
  file.close();
  return size;
}

bool streamRecord(WiFiClient& client, const String& path, String& error) {
  File file = LittleFS.open(path, "r");
  if (!file) {
    error = "Could not open " + path;
    return false;
  }

  RecordHeader header{};
  header.pathLength = static_cast<uint16_t>(path.length());
  header.dataLength = static_cast<uint32_t>(file.size());

  if (!streamExact(client, reinterpret_cast<const uint8_t*>(&header), sizeof(header)) ||
      !streamExact(client, reinterpret_cast<const uint8_t*>(path.c_str()), path.length())) {
    file.close();
    error = "Client disconnected while writing backup header";
    return false;
  }

  uint8_t buffer[512];
  while (file.available()) {
    const size_t got = file.read(buffer, sizeof(buffer));
    if (got == 0 || !streamExact(client, buffer, got)) {
      file.close();
      error = "Client disconnected while writing backup data";
      return false;
    }
  }
  file.close();
  return true;
}

void removeRestoreTemps() {
  LittleFS.remove("/config.restore");
  for (uint8_t slot = 1; slot <= OPENRF_SLOT_COUNT; slot++) {
    String temp = restorePathFor(slotPath(slot));
    if (LittleFS.exists(temp)) LittleFS.remove(temp);
  }
}

bool validateBackup(File& file, BackupHeader& header, String& error) {
  if (!readExact(file, reinterpret_cast<uint8_t*>(&header), sizeof(header))) {
    error = "Backup header is incomplete";
    return false;
  }
  if (memcmp(header.magic, BACKUP_MAGIC, sizeof(BACKUP_MAGIC)) != 0 || header.version != BACKUP_VERSION) {
    error = "Unsupported or invalid OpenRF backup file";
    return false;
  }
  if (header.fileCount == 0 || header.fileCount > OPENRF_SLOT_COUNT + 1) {
    error = "Invalid backup file count";
    return false;
  }

  for (uint16_t index = 0; index < header.fileCount; index++) {
    RecordHeader record{};
    if (!readExact(file, reinterpret_cast<uint8_t*>(&record), sizeof(record))) {
      error = "Backup record header is incomplete";
      return false;
    }
    if (record.pathLength == 0 || record.pathLength > 48 || record.dataLength > MAX_BACKUP_FILE_SIZE) {
      error = "Backup record contains invalid lengths";
      return false;
    }
    String path;
    path.reserve(record.pathLength);
    for (uint16_t i = 0; i < record.pathLength; i++) {
      const int c = file.read();
      if (c < 0) { error = "Backup path is incomplete"; return false; }
      path += static_cast<char>(c);
    }
    if (!allowedPath(path)) {
      error = "Backup contains a forbidden path: " + path;
      return false;
    }
    if (!file.seek(file.position() + record.dataLength, SeekSet)) {
      error = "Backup record data is incomplete";
      return false;
    }
  }
  if (file.position() != file.size()) {
    error = "Backup has trailing or malformed data";
    return false;
  }
  return true;
}
}

size_t backupCalculateSize(uint16_t& fileCount) {
  fileCount = 0;
  size_t total = sizeof(BackupHeader);
  if (includePath("/config.json")) {
    total += recordSize("/config.json");
    fileCount++;
  }
  for (uint8_t slot = 1; slot <= OPENRF_SLOT_COUNT; slot++) {
    const String path = slotPath(slot);
    if (!includePath(path)) continue;
    total += recordSize(path);
    fileCount++;
  }
  return total;
}

bool backupStreamToClient(WiFiClient& client, String& error) {
  uint16_t fileCount = 0;
  backupCalculateSize(fileCount);
  if (fileCount == 0) {
    error = "Nothing to back up";
    return false;
  }

  BackupHeader header{};
  memcpy(header.magic, BACKUP_MAGIC, sizeof(BACKUP_MAGIC));
  header.version = BACKUP_VERSION;
  header.fileCount = fileCount;
  if (!streamExact(client, reinterpret_cast<const uint8_t*>(&header), sizeof(header))) {
    error = "Client disconnected before backup started";
    return false;
  }

  if (includePath("/config.json") && !streamRecord(client, "/config.json", error)) return false;
  for (uint8_t slot = 1; slot <= OPENRF_SLOT_COUNT; slot++) {
    const String path = slotPath(slot);
    if (includePath(path) && !streamRecord(client, path, error)) return false;
  }
  for (uint8_t slot = 1; slot <= 10; slot++) { const String p = rxSlotPath(slot); if (includePath(p) && !streamRecord(client, p, error)) return false; }
  return true;
}

bool backupRestoreFromFile(const char* uploadPath, String& error) {
  File source = LittleFS.open(uploadPath, "r");
  if (!source) {
    error = "Uploaded backup file could not be opened";
    return false;
  }

  BackupHeader header{};
  if (!validateBackup(source, header, error)) {
    source.close();
    return false;
  }

  removeRestoreTemps();
  if (!LittleFS.exists("/slots") && !LittleFS.mkdir("/slots")) {
    source.close();
    error = "Could not create slot directory";
    return false;
  }

  source.seek(sizeof(BackupHeader), SeekSet);
  uint8_t buffer[512];
  for (uint16_t index = 0; index < header.fileCount; index++) {
    RecordHeader record{};
    if (!readExact(source, reinterpret_cast<uint8_t*>(&record), sizeof(record))) {
      error = "Could not read backup record";
      removeRestoreTemps(); source.close(); return false;
    }
    String finalPath;
    finalPath.reserve(record.pathLength);
    for (uint16_t i = 0; i < record.pathLength; i++) finalPath += static_cast<char>(source.read());
    const String tempPath = restorePathFor(finalPath);
    if (tempPath.length() == 0) {
      error = "Invalid restore destination";
      removeRestoreTemps(); source.close(); return false;
    }
    File destination = LittleFS.open(tempPath, "w");
    if (!destination) {
      error = "Could not create temporary restore file";
      removeRestoreTemps(); source.close(); return false;
    }
    uint32_t remaining = record.dataLength;
    while (remaining > 0) {
      const size_t part = remaining > sizeof(buffer) ? sizeof(buffer) : remaining;
      if (!readExact(source, buffer, part) || !writeExact(destination, buffer, part)) {
        destination.close();
        error = "Backup extraction failed";
        removeRestoreTemps(); source.close(); return false;
      }
      remaining -= part;
    }
    destination.close();
  }
  source.close();

  // All files have been extracted successfully. Only now replace live data.
  for (uint8_t slot = 1; slot <= OPENRF_SLOT_COUNT; slot++) {
    const String finalPath = slotPath(slot);
    if (LittleFS.exists(finalPath)) LittleFS.remove(finalPath);
  }
  for (uint8_t slot = 1; slot <= OPENRF_SLOT_COUNT; slot++) {
    const String finalPath = slotPath(slot);
    const String tempPath = restorePathFor(finalPath);
    if (LittleFS.exists(tempPath) && !LittleFS.rename(tempPath, finalPath)) {
      error = "Could not activate restored slot " + String(slot);
      removeRestoreTemps(); return false;
    }
  }
  if (LittleFS.exists("/config.restore")) {
    if (LittleFS.exists("/config.json")) LittleFS.remove("/config.json");
    if (!LittleFS.rename("/config.restore", "/config.json")) {
      error = "Could not activate restored configuration";
      removeRestoreTemps(); return false;
    }
  }
  return true;
}
