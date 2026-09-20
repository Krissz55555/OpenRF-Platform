#!/usr/bin/env python3
"""Host regression runner. Requires Python 3 and g++; no Arduino dependencies."""
import os
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
common = ['src/protocol_engine.cpp', 'src/protocol_registry.cpp',
          'src/normalized_event_dedup.cpp', 'src/v2_actionable_dry_run.cpp',
          'src/protocols/known_protocol_library.cpp',
          'src/protocols/nvkp01_decoder.cpp',
          'src/protocols/ev1527_decoder.cpp', 'src/protocols/pt2262_decoder.cpp',
          'src/protocols/ht12e_decoder.cpp', 'src/protocols/ev1527_tx.cpp',
          'src/protocols/pt2262_tx.cpp', 'src/protocols/ht12e_tx.cpp',
          'src/raw_match.cpp']
tests = ['step40_fix2_nvkp01_structure_test', 'step29_12_nvkp01_decoder_test',
         'step30_four_module_integration_test',
         'step39_1_nvkp01_v2_authoritative_test', 'step40_raw_match_test']
with tempfile.TemporaryDirectory(prefix='openrf-fix2-') as tmp:
    for test in tests:
        exe = str(Path(tmp) / test)
        cmd = [os.environ.get('CXX', 'g++'), '-std=c++11', '-Wall', '-Wextra',
               '-Werror', '-Iinclude', '-Itests', 'tests/' + test + '.cpp',
               *common, '-o', exe]
        subprocess.run(cmd, cwd=root, check=True)
        subprocess.run([exe], cwd=root, check=True)
print('All 5 targeted host suites PASS')
