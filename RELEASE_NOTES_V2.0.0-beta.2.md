# OpenRF Platform v2.0.0-beta.2

V2 Protocol Engine beta release notes, development history and technical reference.

The preceding v2.0.0-beta.1 release introduced the ESP32-S3 N16R8 platform.
This second beta contains the subsequent dual-radio RF architecture, V2-only
protocol path, bidirectional RAW workflow, expanded diagnostics and the
foundations required for the future Deep Analyzer.

This document consolidates the ESP32-S3 port, dual-radio work, Protocol Engine
Steps 22–40 and their fixes, and the final WebUI cleanup. It replaces the separate
V2 Step/FIX notes. The v1.0.0, v1.1.0 and v1.2.0 release notes remain historical
records for the ESP8266 generation.

The functional baseline is Step 40 FIX6. Subsequent Phase A and documentation
changes do not introduce RF features. Beta hardware acceptance is still separate
from source review and host/build checks; this document does not declare every
device test complete.

## 1. Current beta capabilities

| Area | v2.0.0-beta.2 |
| --- | --- |
| Platform | ESP32-S3 N16R8, 16 MB flash, 8 MB PSRAM |
| RF hardware | Two CC1101 receivers, independently enabled; 433.920 and 868.350 MHz defaults |
| Processing | Core 1 RF processing; Core 0 system/network services; queue communication |
| Analyzer | Concurrent with gateway operation; Both / Radio 1 / Radio 2 views |
| Protocol RX | V2-only EV1527/Princeton, PT2262/Tri-State, HT12E and NVKP01 |
| Protocol TX | Modular EV1527, PT2262 and HT12E encoders; NVKP01 is RX-only |
| RF Slots | 30 persistent full-RAW captures, replay plus eligible UNKNOWN RAW receive matching |
| RX Slots | Decoded protocol identities with learned radio/frequency; protocol Send where supported |
| Tuning | Per-radio scanners, carrier estimation, temporary tuning sessions and slot-specific TX retuning |
| Integration | WebUI, REST, MQTT, Home Assistant discovery, OTA firmware, backup/restore |
| LoRa | SX1276 hardware detection only; RF-engine integration pending |
| Deep Analyzer | Planned V2.1 work, not implemented in this beta |

## 2. Development history

The port's Steps 1–7 are a separate sequence from the later RF-development
steps. The source package does not contain a unique numbered note for every
early RF step, nor a distinct Step 20/21 record. The early work below is grouped
by documented milestone rather than inventing missing numbers. Steps 22 onward
retain their recorded numbering. Descriptions in this section are historical;
temporary fallback paths and experimental rules are not current user options.

### ESP32-S3 port — Steps 1–7

1. **Base port:** carried Wi-Fi STA/AP, WebUI/REST, LittleFS, OTA, backup/restore,
   CC1101, Learn/replay, slots, Analyzer, MQTT/HA and Kinetic support from v1.2.0.
2. **ESP8266 restrictions removed:** eliminated the need for exclusive Analyzer
   operation and reviewed the old low-memory protections. Analyzer and gateway
   services can operate concurrently on the S3.
3. **Dual-core separation:** RF RX/TX, capture, Learn and analysis moved into the
   Radio task; network and application services remained in the System domain.
4. **Queue boundary:** commands use `rfCommandQueue`; owned RF event snapshots
   use `rfEventQueue`. Borrowed RAW pointers never cross the core boundary.
5. **Analyzer modernization:** controlled live/full refreshes, request guards
   and candidate presentation reduced stale UI updates and network backlog.
6. **N16R8 memory model:** enabled the intended flash/OPI PSRAM configuration;
   large working buffers prefer PSRAM, critical ISR/queue/task data stays internal.
7. **Stabilization:** RX Learn RSSI filtering, live load/memory diagnostics,
   rejected-candidate safety, RX Slot backup coverage and task-stack headroom.

The original port boundary excluded the second active radio and protocol Send.
Later milestones below implement those features; that old boundary no longer
describes this beta. A later port package was also labelled Step 8; the retained
port summary itself enumerates seven milestones, not eight separate changes.

### Early RF expansion — second radio, scanning and capture visibility

- **Dual CC1101 step 1:** integrated Radio 2 on shared SPI with separate CS/GDO
  pins. Initially the selected band chose one active receiver.
- **868 MHz scanner and scanner v2:** added RSSI sweeping, selectable frequency
  steps, then peak sampling throughout each dwell to catch short transmissions.
- **Normal baseline restoration:** removed the temporary Radio 2 at 433 MHz
  diagnostic configuration; restored Radio 1=433 and Radio 2=868 mapping.
- **Concurrent dual RX:** independent interrupt state and buffers per radio;
  Learn observes enabled receivers and keeps the first valid capture. Slot TX
  selects its radio and pauses receivers to avoid self-capture before restoration.
- **Dual scanners:** one shared scanner implementation serves both radios.
  Default scan ranges are 433.600–434.200 and 867.800–868.900 MHz; the companion
  enabled radio can receive while the selected radio is being scanned.
- **Startup RGB:** boot indication was added for GPIO48/GPIO38 board revisions;
  a compatibility fix used `neopixelWrite()` with the selected Arduino core.
- **Scanner analysis:** quietest-60% noise-floor estimate, signal-above-noise
  measurement and a conservative +12 dB detection indication.
- **Per-radio capture diagnostics:** edges, candidates, accepted/rejected frames
  and current pulse counts made the 868 receive-path problem measurable.
- **Adaptive tuning:** multi-pass peak retention and a 3 dB peak-region weighted
  carrier estimate improved short-burst scanning. The approach was inspired by
  a Sanwa ESP32 contributor's sweep suggestion; implementation is OpenRF-native.
- **RF metadata pipeline:** radio ID, exact frequency and RSSI travel with RAW,
  Analyzer snapshots, Learn and queued events. RX Slot V4 added source metadata;
  legacy V2/V3 slots without it remain frequency-agnostic.
- **Per-radio Analyzer state:** independent candidate, last result, noise and
  counter state prevents one band overwriting the other. Both/R1/R2 views and
  `?radio=1` / `?radio=2` API selection expose that separation.
- **Live UI and direct refresh:** visible diagnostics poll independently of
  Analyzer ON/OFF; URL hashes preserve the selected tab. Guarded 500 ms Analyzer
  refresh replaced an overcomplicated live/sequence/debounce chain.
- **Frame finalization visibility:** captured gap/timeout/buffer-full reasons,
  ignored edges, short-gap resets, pending-frame age and validation results.

### Steps 13–19 — short bursts, latency and regression containment

- **Stale-partial recovery (13/14 sequence):** recovery initially covered
  12–19 pulses, then 6–19 after 75 ms idle. It exposed short bursts as candidates;
  it did not turn them into automatically accepted Analyzer results.
- **Step 15:** measured processing time, API construction and browser request
  latency. A compile fix placed `processingUs` in the correct snapshot structure.
- **Step 16:** serialized `/api/analyzer` into a buffer before sending, replacing
  slow direct JSON streaming. The historical measurement showed roughly 2.7 s
  HTTP time despite negligible processing/API construction time.
- **Step 17:** removed misleading observed-to-UI latency, separated Analyzer
  result age from candidate age, and retained actual browser round-trip timing.
- **Step 18:** restored conservative 433 MHz capture. Short-partial recovery
  became Radio 2-only; no broad RSSI threshold relaxation was introduced.
- **Both-view work (Step 19 context):** made the two-band overview useful without
  mixing detailed RAW data. The retained changelog records the per-radio UI work
  without a standalone numbered Step 19 note.

### Steps 22–28 — frequency ownership and saved slots

| Step | Essential result |
| --- | --- |
| 22 | Separated fixed Default frequency from current Operating frequency per radio; scans restore Operating frequency. |
| 23 | RX Slots persist learned radio ID and exact capture frequency; old slots remain readable. |
| 24 | Protocol RX Slot Send reconstructs a waveform, temporarily tunes the target radio, transmits, then restores its previous Operating frequency. |
| 25 | MQTT and HA Send use the same `rxSlotSend()` path as WebUI, with capability-based discovery. |
| 26 | RAW Slot V2 stores radio/frequency; V1 stays replayable, and rename upgrades its container without changing RF pulses. |
| 26.2 | Per-radio 15-minute tuning sessions, countdown badges, expiry and Restore Default. |
| 26.2.1 | Removed a misplaced expiry check causing a build regression; documented reboot persistence and fresh countdown. |
| 27 | RAW replay uses learned radio/frequency temporarily, restores Operating frequency and preserves tuning sessions. |
| 27.1 | Tune/Restore immediately refresh status and radio UI, avoiding stale badges/frequency displays. |
| 28 | Explicit per-radio capture profiles freeze conservative 433 behavior and short-burst 868 recovery independently. |

### Step 29 — Protocol Engine built beside the working path

| Substep | Essential result |
| --- | --- |
| 29.1 | Immutable, zero-copy `RawCapture` view of the complete finalized capture on Core 1. |
| 29.2 | Synchronous engine hook before validator early returns; initially an empty shadow observer. |
| 29.3 | Static registry with capacity 16 and non-owning decoder pointers; no per-capture allocation. |
| 29.4 | Common independent decoder contract, MATCH/NO_MATCH; no side effects or shared authoritative frame splitter. |
| 29.5 | First conservative EV1527/Princeton shadow decoder. |
| 29.5/2 | Compact same-capture legacy/V2 diagnostics and temporary legacy-action control for comparison. |
| 29.5/3 | Timing, repeat, frame and semantic reject diagnostics without changing thresholds. |
| 29.5/4 | Legacy frame-interpretation measurements isolated the fixed-stride extraction defect. |
| 29.5/5 | Local sync-boundary extraction replaced rigid 50-pulse slicing; leading/trailing partial repeats no longer invalidate an otherwise valid burst. |
| 29.5/6 | Removed application-specific one-hot/address assumptions; required agreeing repeats; tri-state lookalikes are left to PT2262 recognition. |
| 29.6 | Independent PT2262 decoder: 12 trits, base-3 code, local repeat extraction and timing agreement. |
| 29.7 | 0 matches=UNKNOWN, 1=KNOWN, multiple=AMBIGUOUS; no guessed winner. |
| 29.8 | Compact normalized event from one complete KNOWN result: protocol/code/symbols/repeats plus source metadata. |
| 29.9 | Same-capture legacy/V2 comparison classified agreements, mismatches and non-comparable observations. |
| 29.10 | Shadow logical-event dedup keyed by protocol/code/radio with a sliding inactivity window. |
| 29.11 | Actionable dry run reported WOULD_EMIT without touching RX Slots/MQTT/HA. |
| 29.12 | NVKP01 became a third independent shadow module, proving the engine could accept a different RF structure. |

All of Step 29 was staged to protect the original action path. Old diagnostic
switches and broad early NVKP01 marker acceptance are historical, not current.

### Steps 30–35 — V2 authority, learning, transmission and diagnostics

- **30:** a static Known Protocol Library registered EV1527, PT2262, NVKP01 and
  the new HT12E module. HT12E uses its pilot/sync, 12-bit complementary 1:2 timing
  and agreeing repeated words; one isolated word is insufficient.
- **31:** controlled V2 authority for EV1527/PT2262/HT12E. Compact event metadata
  crossed the existing queue; accepted-frame action dedup prevented shadow or
  rejected captures suppressing real actions. Legacy fallback was temporary.
- **32:** V2-first became the boot default; legacy ran only on demand instead of
  duplicating every decode. Temporary rollback remained until migration finished.
- **33:** native V2 RX Slot learning retained code, symbols, repeats, base timing
  and source metadata without a second decode or changing V4 storage.
- **34:** introduced a separate optional `ProtocolTxEncoder` contract and HT12E
  TX. Encoders fill caller-owned pulse buffers; radio access stays in the transport.
- **34 FIX1:** fixed direct-TX restoration, GDO0 direction and RX restart checks.
- **34 FIX2:** bounded CC1101 MISO-ready probing plus successful runtime init
  prevented false OFFLINE status; shared hard-reset/reinitialization recovery
  restored both radio channels after TX.
- **34.4:** once-per-second in-place RX Slot statistics plus a 45-second firmware
  Learn watchdog; the UI waits 50 seconds to observe timeout and unlock controls.
- **35:** separated maintenance in System from RF/developer Diagnostics; retired
  obsolete shadow comparison and dry-run panels.
- **35.1:** a RAM-only last-known-event latch keeps the last recognized protocol,
  code, source and age visible even after subsequent noise/UNKNOWN captures.

### Steps 36–39 — complete modular TX and retire legacy actions

- **36:** EV1527/Princeton TX moved into its V2 module, retaining measured base
  timing and replay count. A pre-transmission compatibility fallback was temporary.
- **37:** PT2262 TX moved into its own module using 12 base-3 packed trits and
  learned timing. Its old encoder fallback was also temporary.
- **38:** removed `protocol_encoder.*` and all generic legacy protocol TX
  fallbacks. Unsupported protocols return unsupported; malformed old metadata
  may require relearning. Low-level tuned RAW transport and RAW replay remain.
- **39.1:** promoted NVKP01 to V2 authority and supported old
  `nvkp01/button/PRESS` slots. The first version retained two-capture confirmation;
  Step 40 FIX3 subsequently replaced it.
- **39.2:** removed general legacy RX fallback, legacy-only rollback and the old
  action switches. UNKNOWN/AMBIGUOUS no longer invoke a legacy protocol action.
- **39.2 FIX1:** restored generic slot path/validation helpers and an explicit
  legacy-format migration include after a compile regression. This did not
  restore legacy RX actions.

Historical generic Analyzer helpers and old on-disk format migration code can
still exist. “Legacy removed” means the protocol RX/TX action fallback paths,
not deletion of every compatibility or diagnostic helper.

### Step 40 — bidirectional RAW and final RF corrections

- **Base Step 40:** saved RAW RF Slots gained receive matching for eligible
  V2-UNKNOWN captures. Full captured pulses remain unchanged for TX replay.
  Prepared signatures are cached in PSRAM-preferred memory instead of rereading
  LittleFS for every frame. Slot statistics, MQTT/HA receive discovery and matcher
  diagnostics were added.
- **FIX1:** kept the strongest comparison candidate even on NO_MATCH, exposing
  timing/count/polarity scores and signature geometry without loosening gates.
- **FIX2:** corrected false NVKP01 recognition of DSC samples. Removed acceptance
  by marker count alone; required related sync, marker and short/long structures.
- **FIX3:** retained sync-based recognition and added a constrained compact-cell
  branch for sync-less genuine captures. Same-sign merging is decoder-local,
  not a mutation of FULL RAW. Removed two-capture confirmation; a confident first
  capture may act. NVKP01 gained 400 ms sliding inactivity dedup in normalized
  and authoritative paths; other known protocols retain 300 ms.
- **FIX4:** added the reinforced two-cell compact variant. It requires marker,
  immediate header, forward cell, a corroborating second marker and adjacent
  reverse cells in the required order. It is not a generic marker-count fallback.
- **FIX5:** corrected a Learn/monitor discrepancy: base-valid short UNKNOWN
  captures could be learned but were filtered before matching. A dedicated
  `RAW_MATCH_CANDIDATE` event now reaches only the Learned RAW matcher; normal
  RX Slots and ordinary MQTT `/rx` telemetry ignore this event. Background
  filtering, KNOWN ownership and AMBIGUOUS rejection remain.
- **FIX6:** the RAW slot's 300 ms dedup clock is refreshed by every match,
  including suppressed repeats. A new event needs more than 300 ms without a
  match for that slot. No DSC-specific rule was introduced.

The FIX4 historical host regression kept 24/29 labelled capture matches,
including duplicates and known damaged genuine rejects; the three DSC negatives
remained UNKNOWN. Synthetic negative tests are useful regressions, not a measured
real-world false-positive rate or independent protocol validation.

### Phase A and final packaging

- Removed public Step/migration wording and corrected RAW/RX Slot explanations,
  two-radio About content and dynamic version display.
- Grouped advanced Analyzer controls and moved protocol-TX diagnostics to
  Diagnostics. LoRa displays pending integration while preserving its saved value.
- **Phase A FIX1:** Dashboard radio state now comes from `/api/status`; the
  nonexistent `/api/radio` online fields no longer produce blank status labels.
- **Phase A FIX2:** restored original live Core 0/Core 1/PSRAM/heap header gauges;
  fixed Diagnostics headings, label/value widths and responsive stacking.
- **Phase A FIX3:** Analyzer no longer leaves an RSSI-filtered received frame at
  an unexplained Waiting state. It shows the measured RSSI and configured limit;
  accepted Analyzer frames display the same authoritative V2 protocol result as
  Diagnostics instead of relying only on the older Analyzer decoder path.
- **Phase A FIX3.1:** a newer RSSI-filtered candidate now supersedes an older
  accepted Analyzer frame in the WebUI. A later accepted frame replaces it again,
  so the displayed state consistently follows the latest received event.
- **Phase A FIX4:** restored comfortable navigation spacing while keeping the
  single-row tablet layout and narrow-screen horizontal scrolling. Home Assistant
  Discovery now removes the legacy `Last RF pulse count` and `Last RF RSSI`
  sensors so raw traffic cannot continuously fill Recorder history. The raw
  MQTT `/rx` stream remains available; protocol, RX Slot and Learned RAW events
  continue to reach Home Assistant unchanged.
- **Compact UI/documentation pass:** reduced vertical card/row spacing and
  retained scrollable navigation on narrow screens, and
  consolidated development MDs into this document. RF and JavaScript unchanged.

## 3. Current RF architecture and frozen decisions

1. Per-radio GDO0 ISR captures signed pulse durations in internal RAM.
2. Core 1 finalizes a full capture and supplies an immutable `RawCapture` view.
3. Independent registered decoders inspect the same full capture, performing
   protocol-local segmentation if needed. No common splitter destroys sync,
   preamble, trailing or repeat evidence for another decoder.
4. The engine aggregates matches. A single complete KNOWN event follows the
   normalized protocol path. AMBIGUOUS never gets a guessed winner.
5. Accepted protocol actions cross the existing queue to RX Slot/MQTT/HA logic.
   A deduplicated KNOWN capture remains protocol-owned, not RAW-match eligible.
6. Only eligible UNKNOWN captures can match saved RAW signatures. Invalid
   captures and active-Learn exclusions remain enforced. Unmatched UNKNOWN
   captures do not create an automation event.

`RawCapture` is borrowed synchronously; do not retain its pointer. Decoders must
not allocate per capture, publish, access storage/network or change radio state.
Cross-core event data is owned. Protocol encoders similarly produce pulses without
performing radio I/O. New modules must remain isolated and independently tested.

### Capture and memory limits

| Setting | Radio 1 | Radio 2 |
| --- | --- | --- |
| Profile | 433 conservative | 868 short-burst |
| Default frequency | 433.920 MHz | 868.350 MHz |
| Frame gap | 25 ms | 25 ms |
| Normal capture minimum | 20 pulses | 20 pulses |
| Short-partial recovery | Disabled | 6–19 pulses after 75 ms idle |

ISR buffers remain 600 pulses per radio. Non-ISR working storage can hold up to
2048 pulses; this is not a 2048-pulse ISR capture capability. Analyzer acceptance
and runtime frame validation are separate from capture finalization. A visible
short candidate is not automatically an accepted actionable signal.

### Protocol transmission contracts

| Protocol | Encoding |
| --- | --- |
| EV1527 | 24 bits MSB-first; 0=HIGH 1T/LOW 3T, 1=HIGH 3T/LOW 1T; sync HIGH 1T/LOW 31T; learned T 180–700 us |
| PT2262 | 12 trits MSB-first; base-3 code; 0=S,L,S,L; 1=L,S,L,S; F=S,L,L,S; L=3T; sync HIGH 1T/LOW 31T; learned T 180–700 us |
| HT12E | T=400 us, LOW 36T pilot, HIGH 1T sync, 12 bits MSB-first; 0=LOW 1T/HIGH 2T, 1=LOW 2T/HIGH 1T; five words |
| NVKP01 | No protocol TX encoder; Send unsupported |

WebUI, MQTT and HA use the same transmit path. The radio is temporarily set to
the learned slot frequency, then restored before RX resumes. Replay transport
is retained even though legacy protocol encoders have been removed.

### Learned RAW matching and events

- Merge same-polarity neighbours in the prepared comparison signature; retain
  the original full capture in slot storage for transmission.
- Detect strongly repeated periods and reduce stable bursts where justified.
- Compare up to 512 signature pulses with polarity, relative timing, pulse-count
  and small start-edge shift checks; current gates include combined score 90%,
  sign agreement 95%, count similarity 80%, and up to two pulses of edge shift.
- Apply slot source radio/frequency checks. The best matching slot emits;
  near-miss diagnostics do not imply acceptance.
- Cache up to 30 signatures, preferring PSRAM with internal-memory fallback;
  reload/clear after save/delete, rebuild after restore reboot.
- RAW dedup is sliding 300 ms per selected slot: exactly 300 ms is suppressed,
  301 ms without a match permits a new event. Unknown/noise does not magically
  merge physically separate packet types into one press.
- Match count, similarity and RSSI are runtime statistics, not durable history.

### NVKP01 technical limits retained from the development notes

The current decoder uses an 18–70-pulse, 18–60 ms input envelope and bounded
local canonicalization. Zero durations/overflow reject. Both mechanical phases
map to code 1, symbol count 1; this is not a decoded unique transmitter identity
or evidence for PRESS/RELEASE payload bits.

Sync-based structure relates S (LOW 5400–6000/HIGH 600–750 us), M (LOW
1400–1750/HIGH 430–730 us) and a following short/long Q pair. S must have the
proper polarity/ratio and be local to M; Q uses 140–280 and 580–720 us with
ratio 2.2–5.0. Marker count or a leader alone cannot authorize an event.

The compact branch uses an early M (LOW 1300–1750/HIGH 600–760 us,
period 2000–2350 us), a related H period of 1250–1500 us and forward/reverse
cells of period 730–930 us. Normal compact recognition requires a forward cell
and at least three reverse cells. The reinforced alternative instead requires
M,H,...,F,...,M2,...,R,R with adjacent R cells; M2 corroborates M within period
ratio 0.90–1.10 and HIGH ratio 0.85–1.15. The source and host fixtures retain
the complete integer bounds and locality checks.

These are sample-derived structural rules, not a complete protocol specification.
Conservative recall can reject genuine damaged/variant captures. The historical
60.74 ms sample remains outside the envelope. Different NVKP01 transmitters may
not be distinguishable by the normalized identity. A long hold/release can
produce separate events after the 400 ms quiet window; rapid genuine presses
can merge. There is no mechanical-button state machine.

## 4. Hardware, tuning and upgrades

Use band-appropriate CC1101 modules and antennas, 3.3 V supply and common ground.

| Signal | GPIO |
| --- | --- |
| Shared CC1101 SCK / MISO / MOSI | 12 / 13 / 11 |
| Radio 1 CS / GDO0 / GDO2 | 10 / 4 / 5 |
| Radio 2 CS / GDO0 / GDO2 | 9 / 6 / 7 |
| SX1276 SCK / MOSI / MISO | 14 / 15 / 16 |
| SX1276 NSS / RST / DIO0 / DIO1 | 17 / 18 / 21 / 2 |

System Tune starts a 15-minute session per radio. Learned slots capture the
actual Operating frequency. Reboot restores a saved tuned frequency with a fresh
15-minute window; expiry or Restore Default persists the default. Scans and
temporary TX do not take ownership of the session. Radio 1 and 2 timers are
independent. Software frequency selection cannot compensate for an unsuitable
antenna or RF matching network.

Build using the `esp32s3` PlatformIO environment. Initial installation needs
firmware and LittleFS. Later HTML/JS/CSS updates need a filesystem upload;
documentation-only updates need no device upload. Firmware OTA on System accepts
`firmware.bin`, not `littlefs.bin`, and does not update WebUI assets.

Back up first: filesystem upload replaces LittleFS and can erase settings and
slots. `.orfbackup` includes configuration and RF/RX slots; it contains credentials
and should be kept private. Restore validates the container before applying it and
reboots; RAW signature caches rebuild. Do not assume an ESP8266 backup's radio
configuration is an S3 wiring/setup substitute; check both radio cards afterward.

RAW V1 and RX V2/V3 compatibility is retained. Older slots cannot recover source
metadata that was never stored. Invalid old EV1527/PT2262 TX metadata now requires
relearning rather than a silent legacy encoder fallback. The NVKP01 legacy
identity bridge remains for existing `nvkp01/button/PRESS` slots.

## 5. MQTT, Home Assistant and API reference

The default topic examples below use `openrf/<hostname>`; use your configured base.

| Topic | Purpose |
| --- | --- |
| `.../slot/<n>/send` | Send stored RAW slot (any command payload) |
| `.../slot/<n>/event` | Learned RAW receive match; source `learned_raw`, similarity/RSSI and slot metadata |
| `.../rxslot/<n>/send` | Send supported protocol slot; same path as WebUI |
| `.../rxslot/<n>/send/state` | Result including sent/error/empty/unsupported and available metadata |
| `.../rxslot/<n>/event` | Protocol RX Slot event |
| `.../v2/event` | General authoritative V2 protocol event |

Used RAW slots support Send/Relearn/Delete controls and receive device triggers
and a one-second binary sensor. Protocol RX slots expose receive discovery and
Send only when supported. Slot save/rename/delete and MQTT reconnect update
discovery; deleted entries are cleared. Home Assistant does not create persistent
sensors for every raw frame: the high-rate `/rx` pulse-count and RSSI data remain
available to direct MQTT consumers, while HA Discovery exposes actionable known
protocol, RX Slot and Learned RAW events. Analyzer recognition labels can differ
from authoritative V2 decisions; use Diagnostics to understand the actual action
route.

| HTTP endpoint | Purpose |
| --- | --- |
| `GET /api/status` | Version, hardware online/enabled/active, frequencies, memory/load |
| `GET /api/radio` | Radio/capture and Protocol Engine diagnostics |
| `GET /api/radio/raw` | Latest RAW capture |
| `GET /api/debug/radio` | Detailed radio counters |
| `GET /api/radio/frequency-scan` | Frequency sweep |
| `POST /api/radio/frequency-tune`, `/api/radio/frequency-restore` | Tune/restore selected radio |
| `GET /api/radio/learn`, `/api/radio/learn/raw` | Learn state/preview |
| `POST /api/radio/learn/start`, `/accept`, `/discard`, `/test-send` | Learn actions (suffixes relative to `/api/radio/learn`) |
| `GET /api/slots`, `/api/slots/stats` | RAW slot inventory/runtime stats |
| `POST /api/slots/save`, `/send`, `/rename`, `/delete` | RAW slot actions (suffixes relative to `/api/slots`) |
| `GET /api/rxslots` | Protocol slots, Learn and TX diagnostics |
| `POST /api/rxslots/learn`, `/learn-rssi`, `/send`, `/rename`, `/enable`, `/delete` | Protocol slot actions |
| `GET /api/analyzer`, `/api/analyzer/live`; `POST /api/analyzer/settings` | Analyzer snapshots/settings |
| `GET/POST /api/config`; `POST /api/system/radios` | Configuration/radio enable |
| `GET /api/system/backup`; `POST /api/system/restore`, `/api/system/ota` | Backup, multipart restore and firmware OTA |

## 6. Known limitations and deferred work

- Beta validation depends on real transmitters, receivers and RF conditions;
  successful host tests/builds alone do not establish field reliability.
- DSC and other multi-packet UNKNOWN devices can generate several logical events
  from one physical action when matching packets are separated beyond 300 ms or
  use different structures. “One press = one event” is not a universal guarantee.
- Per-slot configurable cooldown is a future option, not a setting in this beta.
- RAW matching does not decode payload, infer open/closed/tamper, mask changing
  bits, group packet families or implement rolling-code/encryption support.
- Deep Analyzer, autonomous analysis/profile optimization and LoRa RF-engine
  integration are future work. The current Analyzer is not Deep Analyzer.
- Two enabled radios can receive concurrently; intentional TX pauses and shared
  resources mean this is not a claim of unlimited load or zero dropped events.
- Dependencies in `platformio.ini` are unpinned. Rebuilding later can select
  different library/platform versions; record actual versions when reporting bugs.

## 7. Validation and release acceptance

The baseline through Phase A FIX3.1 passed fresh ESP32-S3 firmware and LittleFS
builds, JavaScript syntax, HTML ID/DOM-reference checks and the complete targeted
Step 40 FIX6 host regression chain. The tester subsequently reported the physical
radio and browser checks successful and accepted that baseline as beta-ready.

Phase A FIX4 changes only MQTT Discovery policy and navigation spacing. JavaScript
syntax, static Discovery assertions, Markdown checks and the complete Step 40 FIX6
host regression chain pass. The final FIX4 firmware and LittleFS images were built
and the tester reported the complete device, WebUI, radio, MQTT and Home Assistant
test set successful. The tested FIX4 source and those exact images form the public
v2.0.0-beta.2 release candidate.

For the current RF baseline, `python3 tests/run_step40_fix6_host.py` runs the
targeted host regression chain. Python 3 and g++ are required. Historical runners
may encode superseded expectations, notably NVKP01 confirmation/recognition;
use the current chain rather than treating every old script as a current gate.
Retained fixtures and test source are not removed during MD consolidation.

Before publication check:

- Firmware build, buildfs, first boot and upgrade with the actual board.
- Both radio modules, known 433/868 reception and post-TX RX recovery.
- EV1527/PT2262/HT12E RX and protocol Send; NVKP01 reception without false actions.
- UNKNOWN RAW Learn, save, replay, receive match, dedup and KNOWN isolation.
- MQTT events, Home Assistant discovery/triggers and reconnect.
- Analyzer alongside gateway traffic; both scanners, Tune, expiry and restoration.
- Slot/settings persistence after reboot; backup/restore and firmware OTA.
- Compact desktop/mobile pages, gauges, menu scrolling and Diagnostics long values.
- Extended operation without resets, progressive memory loss or stuck Learn state.

The generated release asset names are
`OpenRF-Platform-v2.0.0-beta.2-source.zip`,
`OpenRF-Platform-v2.0.0-beta.2-ESP32S3-firmware.bin` and
`OpenRF-Platform-v2.0.0-beta.2-ESP32S3-littlefs.bin`.

## 8. Consolidation map

| Former notes | Destination in this document |
| --- | --- |
| `ESP32_S3_PORT.md` | Port Steps 1–7 and current memory/upgrade boundary |
| V2 portions of `CHANGELOG.md` | Early RF expansion, Steps 13–19 and numbered history |
| `docs/STEP22*` through `docs/STEP28*` | Frequency ownership table and tuning/upgrade reference |
| Root `STEP29*`, `docs/STEP29*` | Complete Step 29 substep table and architecture rules |
| Root/docs `STEP30*`, `STEP31*`, `STEP32*` | Shadow-to-authoritative migration history |
| Root `STEP33*` through `STEP39*` | Native Learn/TX, recovery, Diagnostics and legacy retirement |
| Root `STEP40*` | Bidirectional RAW, FIX1–FIX6, NVKP01 limits and event semantics |
| `ANALYZER_V2_FINAL.md` | Analyzer capability summary; v1.1 release notes retain historical mode descriptions |
| Old subsystem docs and `docs/VERSION_HISTORY.md` | Inherited milestone overview in CHANGELOG; current usage in README and sections 3–6 here |
| `src/protocols/kinetic/nvkp01/README.md`, `notes.md` | NVKP01 history, measured structure and explicit limits; sample data/source remain in place |

The original input ZIP remains the full historical archive. Duplicate notes and
obsolete intermediate instructions are omitted only from this consolidated copy.
