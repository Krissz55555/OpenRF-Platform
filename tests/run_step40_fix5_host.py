#!/usr/bin/env python3
from pathlib import Path
import subprocess

root = Path(__file__).resolve().parents[1]
build = root / "tests" / ".build_step40_fix5"
build.mkdir(exist_ok=True)

binary = build / "step40_fix5_short_raw_route_test"
subprocess.run([
    "g++", "-std=c++17", "-Wall", "-Wextra", "-Werror",
    "-I", str(root / "include"),
    str(root / "tests" / "step40_fix5_short_raw_route_test.cpp"),
    str(root / "src" / "short_raw_candidate_route.cpp"),
    "-o", str(binary),
], check=True)
subprocess.run([str(binary)], check=True)

dualcore = (root / "include" / "dualcore.h").read_text(encoding="utf-8")
radio = (root / "src" / "radio.cpp").read_text(encoding="utf-8")
matcher = (root / "src" / "raw_slot_matcher.cpp").read_text(encoding="utf-8")
mqtt = (root / "src" / "mqtt.cpp").read_text(encoding="utf-8")

assert "RAW_MATCH_CANDIDATE" in dualcore
assert "RFEventType::RAW_MATCH_CANDIDATE" in radio
assert "ProtocolEngineDecisionState::UNKNOWN" in radio
assert 'rejectReason == "background_short_frame"' in radio
assert "RFEventType::RAW_MATCH_CANDIDATE" in matcher
assert "event.type == RFEventType::RX_FRAME" in mqtt

# Run the complete prior FIX4 regression after the new route test.
subprocess.run(["python3", str(root / "tests" / "run_step40_fix4_host.py")],
               cwd=root, check=True)
print("All targeted FIX5 host suites PASS")
