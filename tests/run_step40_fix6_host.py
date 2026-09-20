#!/usr/bin/env python3
from pathlib import Path
import subprocess

root = Path(__file__).resolve().parents[1]
build = root / "tests" / ".build_step40_fix6"
build.mkdir(exist_ok=True)

binary = build / "step40_fix6_sliding_raw_dedup_test"
subprocess.run([
    "g++", "-std=c++17", "-Wall", "-Wextra", "-Werror",
    "-I", str(root / "include"),
    str(root / "tests" / "step40_fix6_sliding_raw_dedup_test.cpp"),
    str(root / "src" / "raw_slot_dedup.cpp"),
    "-o", str(binary),
], check=True)
subprocess.run([str(binary)], check=True)

matcher = (root / "src" / "raw_slot_matcher.cpp").read_text(encoding="utf-8")
dedup = (root / "src" / "raw_slot_dedup.cpp").read_text(encoding="utf-8")
assert "rawSlotDedupShouldEmit(best.lastEmitAtMs, now)" in matcher
assert "lastSeenAtMs = nowMs" in dedup
assert "DSC" not in dedup

# Preserve every FIX5 routing and earlier protocol/matcher regression.
subprocess.run(["python3", str(root / "tests" / "run_step40_fix5_host.py")],
               cwd=root, check=True)
print("All targeted FIX6 host suites PASS")
