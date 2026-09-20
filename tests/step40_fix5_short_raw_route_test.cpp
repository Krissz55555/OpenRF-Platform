#include <cassert>
#include <cstdio>

#include "short_raw_candidate_route.h"

int main() {
  // DSC-like capture: accepted during Learn, filtered during monitoring, and
  // now eligible only for the Learned RAW matcher when V2 says UNKNOWN.
  assert(shortRawMatchCandidateEligible(true, false, true, 50U, 48527U));

  assert(!shortRawMatchCandidateEligible(false, false, true, 50U, 48527U));
  assert(!shortRawMatchCandidateEligible(true, true, true, 50U, 48527U));
  assert(!shortRawMatchCandidateEligible(true, false, false, 50U, 48527U));

  // The original background filter is an AND gate: either boundary is enough
  // to keep the frame on the normal accepted path instead of this bypass.
  assert(!shortRawMatchCandidateEligible(true, false, true, 60U, 48527U));
  assert(!shortRawMatchCandidateEligible(true, false, true, 50U, 80000U));

  std::puts("FIX5 short UNKNOWN Learned RAW route: PASS");
}
