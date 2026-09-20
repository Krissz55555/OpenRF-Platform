#include <Arduino.h>
#include <LittleFS.h>
#include "storage.h"
#include "scratch.h"

namespace {
constexpr uint32_t SLOT_MAGIC = 0x4F524653UL; // ORFS
constexpr uint16_t SLOT_FORMAT_VERSION_V1 = 1;
constexpr uint16_t SLOT_FORMAT_VERSION = 2;

#pragma pack(push, 1)
struct SlotHeaderV1 {
  uint32_t magic;
  uint16_t version;
  uint16_t pulseCount;
  uint32_t durationUs;
  uint32_t frequencyHz;
  uint32_t fingerprint;
  char name[OPENRF_SLOT_NAME_MAX + 1];
};

struct SlotHeaderV2 {
  uint32_t magic;
  uint16_t version;
  uint16_t pulseCount;
  uint32_t durationUs;
  uint32_t frequencyHz;
  uint8_t radioId;
  uint32_t fingerprint;
  char name[OPENRF_SLOT_NAME_MAX + 1];
};
#pragma pack(pop)

struct DecodedHeader {
  uint16_t version = 0;
  uint16_t pulseCount = 0;
  uint32_t durationUs = 0;
  uint32_t frequencyHz = 0;
  uint8_t radioId = 0;
  uint32_t fingerprint = 0;
  char name[OPENRF_SLOT_NAME_MAX + 1] = {0};
  size_t headerSize = 0;
};

bool validSlot(uint8_t slot) { return slot >= 1 && slot <= OPENRF_SLOT_COUNT; }
String slotPath(uint8_t slot) { return "/slots/slot" + String(slot) + ".bin"; }
String defaultName(uint8_t slot) { return "RF Slot " + String(slot); }

bool decodeHeader(File& file, DecodedHeader& out) {
  if (!file) return false;
  uint32_t magic = 0;
  uint16_t version = 0;
  if (file.read(reinterpret_cast<uint8_t*>(&magic), sizeof(magic)) != sizeof(magic) ||
      file.read(reinterpret_cast<uint8_t*>(&version), sizeof(version)) != sizeof(version)) return false;
  if (magic != SLOT_MAGIC) return false;
  if (!file.seek(0)) return false;

  if (version == SLOT_FORMAT_VERSION_V1) {
    SlotHeaderV1 h{};
    if (file.read(reinterpret_cast<uint8_t*>(&h), sizeof(h)) != sizeof(h)) return false;
    out.version = h.version;
    out.pulseCount = h.pulseCount;
    out.durationUs = h.durationUs;
    out.frequencyHz = h.frequencyHz;
    out.radioId = 0; // Legacy slot: RF source was not stored.
    out.fingerprint = h.fingerprint;
    memcpy(out.name, h.name, sizeof(h.name));
    out.headerSize = sizeof(h);
  } else if (version == SLOT_FORMAT_VERSION) {
    SlotHeaderV2 h{};
    if (file.read(reinterpret_cast<uint8_t*>(&h), sizeof(h)) != sizeof(h)) return false;
    out.version = h.version;
    out.pulseCount = h.pulseCount;
    out.durationUs = h.durationUs;
    out.frequencyHz = h.frequencyHz;
    out.radioId = (h.radioId == 1 || h.radioId == 2) ? h.radioId : 0;
    out.fingerprint = h.fingerprint;
    memcpy(out.name, h.name, sizeof(h.name));
    out.headerSize = sizeof(h);
  } else {
    return false;
  }

  if (out.pulseCount == 0 || out.pulseCount > OPENRF_MAX_RAW_PULSES) return false;
  out.name[OPENRF_SLOT_NAME_MAX] = '\0';
  return true;
}

bool readHeader(uint8_t slot, DecodedHeader& header) {
  if (!validSlot(slot)) return false;
  File file = LittleFS.open(slotPath(slot), "r");
  if (!file) return false;
  const bool ok = decodeHeader(file, header);
  file.close();
  return ok;
}
}

uint32_t storageFingerprint(const int16_t* pulses, uint16_t pulseCount) {
  uint32_t hash = 2166136261UL;
  for (uint16_t i = 0; i < pulseCount; i++) {
    const uint16_t value = static_cast<uint16_t>(pulses[i]);
    hash ^= static_cast<uint8_t>(value & 0xFF); hash *= 16777619UL;
    hash ^= static_cast<uint8_t>(value >> 8); hash *= 16777619UL;
  }
  hash ^= static_cast<uint8_t>(pulseCount & 0xFF); hash *= 16777619UL;
  hash ^= static_cast<uint8_t>(pulseCount >> 8); hash *= 16777619UL;
  return hash;
}

bool storageSlotExists(uint8_t slot) {
  DecodedHeader header{};
  return readHeader(slot, header);
}

SlotInfo storageGetSlotInfo(uint8_t slot) {
  SlotInfo info;
  info.id = slot;
  info.name = defaultName(slot);
  DecodedHeader header{};
  if (!readHeader(slot, header)) return info;
  info.used = true;
  info.name = strlen(header.name) ? String(header.name) : defaultName(slot);
  info.frequencyMHz = header.frequencyHz / 1000000.0F;
  info.radioId = header.radioId;
  info.pulseCount = header.pulseCount;
  info.durationUs = header.durationUs;
  info.fingerprint = header.fingerprint;
  return info;
}

bool storageSaveSlot(uint8_t slot, const String& requestedName, float frequencyMHz,
                     uint8_t radioId, const int16_t* pulses, uint16_t pulseCount,
                     uint32_t durationUs, uint32_t* fingerprintOut) {
  if (!validSlot(slot) || !pulses || pulseCount == 0 || pulseCount > OPENRF_MAX_RAW_PULSES) return false;
  if (radioId != 0 && radioId != 1 && radioId != 2) return false;
  if (!LittleFS.exists("/slots") && !LittleFS.mkdir("/slots")) return false;

  String name = requestedName;
  name.trim();
  if (name.length() == 0) name = defaultName(slot);
  if (name.length() > OPENRF_SLOT_NAME_MAX) name = name.substring(0, OPENRF_SLOT_NAME_MAX);

  SlotHeaderV2 header{};
  header.magic = SLOT_MAGIC;
  header.version = SLOT_FORMAT_VERSION;
  header.pulseCount = pulseCount;
  header.durationUs = durationUs;
  header.frequencyHz = static_cast<uint32_t>(frequencyMHz * 1000000.0F + 0.5F);
  header.radioId = radioId;
  header.fingerprint = storageFingerprint(pulses, pulseCount);
  name.toCharArray(header.name, sizeof(header.name));

  const String tempPath = slotPath(slot) + ".tmp";
  File file = LittleFS.open(tempPath, "w");
  if (!file) return false;
  bool ok = file.write(reinterpret_cast<const uint8_t*>(&header), sizeof(header)) == sizeof(header);
  if (ok) ok = file.write(reinterpret_cast<const uint8_t*>(pulses), pulseCount * sizeof(int16_t)) == pulseCount * sizeof(int16_t);
  file.close();
  if (!ok) { LittleFS.remove(tempPath); return false; }
  LittleFS.remove(slotPath(slot));
  if (!LittleFS.rename(tempPath, slotPath(slot))) { LittleFS.remove(tempPath); return false; }
  if (fingerprintOut) *fingerprintOut = header.fingerprint;
  return true;
}

bool storageLoadSlot(uint8_t slot, int16_t* pulses, uint16_t capacity, SlotInfo& info) {
  if (!validSlot(slot) || !pulses) return false;
  File file = LittleFS.open(slotPath(slot), "r");
  if (!file) return false;
  DecodedHeader header{};
  if (!decodeHeader(file, header) || header.pulseCount > capacity) { file.close(); return false; }
  if (!file.seek(header.headerSize)) { file.close(); return false; }
  const size_t bytes = header.pulseCount * sizeof(int16_t);
  if (static_cast<size_t>(file.read(reinterpret_cast<uint8_t*>(pulses), bytes)) != bytes) { file.close(); return false; }
  file.close();
  const uint32_t actualFingerprint = storageFingerprint(pulses, header.pulseCount);
  if (actualFingerprint != header.fingerprint) return false;
  info.id = slot; info.used = true;
  info.name = strlen(header.name) ? String(header.name) : defaultName(slot);
  info.frequencyMHz = header.frequencyHz / 1000000.0F;
  info.radioId = header.radioId;
  info.pulseCount = header.pulseCount;
  info.durationUs = header.durationUs;
  info.fingerprint = header.fingerprint;
  return true;
}

bool storageRenameSlot(uint8_t slot, const String& requestedName) {
  if (!validSlot(slot)) return false;
  SlotInfo info;
  if (!storageLoadSlot(slot, openrfScratch, OPENRF_MAX_RAW_PULSES, info)) return false;
  // Renaming a V1 slot upgrades it safely to V2 while preserving its legacy
  // unspecified radioId=0 and all RF data.
  return storageSaveSlot(slot, requestedName, info.frequencyMHz, info.radioId,
                         openrfScratch, info.pulseCount, info.durationUs, nullptr);
}

bool storageDeleteSlot(uint8_t slot) {
  if (!validSlot(slot)) return false;
  const String path = slotPath(slot);
  return !LittleFS.exists(path) || LittleFS.remove(path);
}

uint8_t storageCountUsedSlots() {
  uint8_t count = 0;
  for (uint8_t slot = 1; slot <= OPENRF_SLOT_COUNT; slot++) {
    if (storageGetSlotInfo(slot).used) count++;
    yield();
  }
  return count;
}
