#pragma once

#include <stdint.h>

// Keep the normal short-background filter intact, but identify the narrow
// case which may still be useful to a pre-existing Learned RAW slot.
bool shortRawMatchCandidateEligible(bool passedBaseValidation,
                                    bool learningNow,
                                    bool v2Unknown,
                                    uint16_t pulseCount,
                                    uint32_t durationUs);
