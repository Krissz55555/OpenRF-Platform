#pragma once

#include <stdint.h>

static const uint32_t OPENRF_RAW_SLOT_DEDUP_MS = 300UL;

// Sliding inactivity gate for repeated matches of one learned RAW slot.
// Every observed match refreshes lastSeenAtMs, including suppressed repeats.
// This is deliberately protocol-agnostic; burst interpretation belongs to the
// future Deep Analyzer.
bool rawSlotDedupShouldEmit(uint32_t& lastSeenAtMs, uint32_t nowMs);
