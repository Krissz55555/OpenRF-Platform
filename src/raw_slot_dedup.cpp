#include "raw_slot_dedup.h"

bool rawSlotDedupShouldEmit(uint32_t& lastSeenAtMs, uint32_t nowMs) {
  const bool suppress =
      lastSeenAtMs != 0U &&
      static_cast<uint32_t>(nowMs - lastSeenAtMs) <=
          OPENRF_RAW_SLOT_DEDUP_MS;
  lastSeenAtMs = nowMs;
  return !suppress;
}
