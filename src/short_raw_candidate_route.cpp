#include "short_raw_candidate_route.h"

namespace {
constexpr uint16_t kMonitorMinimumPulses = 60U;
constexpr uint32_t kMonitorMinimumDurationUs = 80000UL;
}

bool shortRawMatchCandidateEligible(bool passedBaseValidation,
                                    bool learningNow,
                                    bool v2Unknown,
                                    uint16_t pulseCount,
                                    uint32_t durationUs) {
  return passedBaseValidation && !learningNow && v2Unknown &&
         pulseCount < kMonitorMinimumPulses &&
         durationUs < kMonitorMinimumDurationUs;
}
