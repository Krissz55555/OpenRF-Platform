#include <cassert>
#include <cstdio>
#include <cstdint>

#include "raw_slot_dedup.h"

int main() {
  uint32_t lastSeen = 0U;

  // One continuous generic burst may last far beyond 300 ms. As long as each
  // repeated match arrives inside the inactivity window, only the first emits.
  assert(rawSlotDedupShouldEmit(lastSeen, 1000U));
  assert(!rawSlotDedupShouldEmit(lastSeen, 1200U));
  assert(!rawSlotDedupShouldEmit(lastSeen, 1450U));
  assert(!rawSlotDedupShouldEmit(lastSeen, 1750U));  // inclusive boundary

  // A new logical event becomes possible only after more than 300 ms silence.
  assert(rawSlotDedupShouldEmit(lastSeen, 2051U));

  // Preserve unsigned wrap-around behavior used by millis().
  lastSeen = UINT32_MAX - 100U;
  assert(!rawSlotDedupShouldEmit(lastSeen, 100U));
  assert(rawSlotDedupShouldEmit(lastSeen, 500U));

  std::puts("FIX6 sliding Learned RAW dedup: PASS");
}
