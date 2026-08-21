const $ = (id) => document.getElementById(id);
const elements = {
  connectionBadge: $("connectionBadge"), status: $("status"), firmware: $("firmware"), ip: $("ip"), wifiMode: $("wifiMode"),
  radioChip: $("radioChip"), radioMode: $("radioMode"), radioFrequency: $("radioFrequency"), radioRssi: $("radioRssi"), mqttStatus: $("mqttStatus"), freeHeap: $("freeHeap"), heapFragmentation: $("heapFragmentation"), uptime: $("uptime"),
  rawPulseCount: $("rawPulseCount"), rawDuration: $("rawDuration"), rawRssi: $("rawRssi"), rawSequence: $("rawSequence"), rawAge: $("rawAge"), rawPreview: $("rawPreview"),
  refreshStatusButton: $("refreshStatusButton"), settingsForm: $("settingsForm"), hostname: $("hostname"), replayCount: $("replayCount"), radioBand: $("radioBand"), wifiSsid: $("wifiSsid"), wifiPassword: $("wifiPassword"), wifiPasswordState: $("wifiPasswordState"), mqttEnabled: $("mqttEnabled"), mqttFields: $("mqttFields"), mqttHost: $("mqttHost"), mqttPort: $("mqttPort"), mqttUser: $("mqttUser"), mqttPassword: $("mqttPassword"), homeAssistantDiscovery: $("homeAssistantDiscovery"), passwordState: $("passwordState"), saveMessage: $("saveMessage"), saveButton: $("saveButton"),
  analyzerSequence: $("analyzerSequence"), analyzerBadge: $("analyzerBadge"), analyzerModeMessage: $("analyzerModeMessage"), analyzerFrequency: $("analyzerFrequency"), analyzerRssi: $("analyzerRssi"), analyzerProtocol: $("analyzerProtocol"), analyzerEncoding: $("analyzerEncoding"), analyzerBits: $("analyzerBits"), analyzerBasePulse: $("analyzerBasePulse"), analyzerFrameCount: $("analyzerFrameCount"), analyzerQuality: $("analyzerQuality"), analyzerPulseCount: $("analyzerPulseCount"), analyzerDuration: $("analyzerDuration"), analyzerCode: $("analyzerCode"), analyzerClasses: $("analyzerClasses"), analyzerCandidates: $("analyzerCandidates"), analyzerAccepted: $("analyzerAccepted"), analyzerRejected: $("analyzerRejected"), analyzerDecoded: $("analyzerDecoded"), analyzerUnknown: $("analyzerUnknown"), analyzerDetailTitle: $("analyzerDetailTitle"), analyzerAge: $("analyzerAge"), analyzerBitstream: $("analyzerBitstream"), refreshAnalyzerButton: $("refreshAnalyzerButton"), copyAnalyzerButton: $("copyAnalyzerButton"), analyzerRssiThreshold: $("analyzerRssiThreshold"), analyzerRssiThresholdValue: $("analyzerRssiThresholdValue"), analyzerRssiSaveState: $("analyzerRssiSaveState"), analyzerCurrentRssi: $("analyzerCurrentRssi"), analyzerPeakRssi: $("analyzerPeakRssi"), analyzerWeakRssi: $("analyzerWeakRssi"), analyzerMinPulses: $("analyzerMinPulses"), analyzerMinPulsesValue: $("analyzerMinPulsesValue"), analyzerMinDuration: $("analyzerMinDuration"), analyzerMinDurationValue: $("analyzerMinDurationValue"), analyzerSimilarity: $("analyzerSimilarity"), analyzerSimilarityValue: $("analyzerSimilarityValue"), analyzerOccurrences: $("analyzerOccurrences"), analyzerOccurrencesValue: $("analyzerOccurrencesValue"), analyzerShowRejected: $("analyzerShowRejected"), analyzerFreezeCandidate: $("analyzerFreezeCandidate"), analyzerAlternation: $("analyzerAlternation"), analyzerAlternationValue: $("analyzerAlternationValue"), analyzerDeveloperMode: $("analyzerDeveloperMode"), developerModeState: $("developerModeState"), developerExclusiveWarning: $("developerExclusiveWarning"), candidateDeveloperMetrics: $("candidateDeveloperMetrics"), candidateAlternation: $("candidateAlternation"), candidateSamePairs: $("candidateSamePairs"), candidateLongestRun: $("candidateLongestRun"), candidateNormalizedCount: $("candidateNormalizedCount"), candidateNormalizedRaw: $("candidateNormalizedRaw"), candidateSequence: $("candidateSequence"), candidateAge: $("candidateAge"), candidateRssi: $("candidateRssi"), candidatePulseCount: $("candidatePulseCount"), candidateDuration: $("candidateDuration"), candidateRejectReason: $("candidateRejectReason"), candidateRaw: $("candidateRaw"),
  learnState: $("learnState"), learnBadge: $("learnBadge"), learnMessage: $("learnMessage"), learnStartButton: $("learnStartButton"), learnAcceptButton: $("learnAcceptButton"), learnSaveButton: $("learnSaveButton"), learnTestSendButton: $("learnTestSendButton"), learnDiscardButton: $("learnDiscardButton"), learnPulseCount: $("learnPulseCount"), learnDuration: $("learnDuration"), learnRssi: $("learnRssi"), learnNoiseFloor: $("learnNoiseFloor"), learnRejected: $("learnRejected"), learnRejectReason: $("learnRejectReason"), learnPreviewTitle: $("learnPreviewTitle"), learnRawPreview: $("learnRawPreview"),
  refreshSlotsButton: $("refreshSlotsButton"), slotUsage: $("slotUsage"), slotMessage: $("slotMessage"), slotGrid: $("slotGrid"),
  refreshRxSlotsButton: $("refreshRxSlotsButton"), rxSlotUsage: $("rxSlotUsage"), rxSlotMessage: $("rxSlotMessage"), rxSlotGrid: $("rxSlotGrid"),
  rxLearnRssiThreshold: $("rxLearnRssiThreshold"), rxLearnRssiValue: $("rxLearnRssiValue"), rxLearnRssiSaveState: $("rxLearnRssiSaveState"),
  slotSaveModal: $("slotSaveModal"), slotSaveSelect: $("slotSaveSelect"), slotSaveName: $("slotSaveName"), slotSaveWarning: $("slotSaveWarning"), slotSaveCancel: $("slotSaveCancel"), slotSaveConfirm: $("slotSaveConfirm"),
  otaFile: $("otaFile"), otaUploadButton: $("otaUploadButton"), otaProgress: $("otaProgress"), otaMessage: $("otaMessage"),
  backupFile: $("backupFile"), backupRestoreButton: $("backupRestoreButton"), backupProgress: $("backupProgress"), backupMessage: $("backupMessage"), uiVersion: $("uiVersion"),
  memoryDiagState: $("memoryDiagState"), memoryFlashTotal: $("memoryFlashTotal"),
  memoryPsramTotal: $("memoryPsramTotal"), memoryPsramFree: $("memoryPsramFree"),
  memoryHeapTotal: $("memoryHeapTotal"), memoryHeapFree: $("memoryHeapFree"), memoryOpenrfPsram: $("memoryOpenrfPsram"), memoryAnalyzerPsram: $("memoryAnalyzerPsram"),
  core0Gauge: $("core0Gauge"), core0Load: $("core0Load"), core1Gauge: $("core1Gauge"), core1Load: $("core1Load"),
  psramGauge: $("psramGauge"), psramLoad: $("psramLoad"), heapGauge: $("heapGauge"), heapLoad: $("heapLoad")
};



function formatMemoryBytes(value) {
  const bytes = Number(value || 0);
  if (!bytes) return "0 MB";
  return `${(bytes / (1024 * 1024)).toFixed(2)} MB`;
}

function renderMemoryDiagnostics(d) {
  if (elements.memoryFlashTotal) elements.memoryFlashTotal.textContent = formatMemoryBytes(d.flash_total);
  if (elements.memoryPsramTotal) elements.memoryPsramTotal.textContent = formatMemoryBytes(d.psram_total);
  if (elements.memoryPsramFree) elements.memoryPsramFree.textContent = formatMemoryBytes(d.psram_free);
  if (elements.memoryHeapTotal) elements.memoryHeapTotal.textContent = formatMemoryBytes(d.heap_total);
  if (elements.memoryHeapFree) elements.memoryHeapFree.textContent = formatMemoryBytes(d.free_heap);
  if (elements.memoryOpenrfPsram) elements.memoryOpenrfPsram.textContent =
    `${formatMemoryBytes(d.openrf_psram_buffers)} · ${d.openrf_psram_external ? "PSRAM" : "fallback"}`;
  if (elements.memoryAnalyzerPsram) elements.memoryAnalyzerPsram.textContent =
    `${formatMemoryBytes(d.analyzer_psram_buffers)} · ${d.analyzer_psram_external ? "PSRAM" : "fallback"}`;
  if (elements.memoryDiagState) {
    const ok = Number(d.psram_total || 0) >= 7 * 1024 * 1024;
    elements.memoryDiagState.textContent = ok ? "PSRAM detected" : "PSRAM not detected";
    elements.memoryDiagState.className = `form-message ${ok ? "success" : "error"}`;
  }
}

function setGauge(gauge, label, value) {
  const p = Math.max(0, Math.min(100, Number(value) || 0));
  if (gauge) gauge.style.setProperty("--p", p.toFixed(0));
  if (label) label.textContent = `${p.toFixed(0)}%`;
}

function setConnectionBadge(state, text) { elements.connectionBadge.className = `badge badge-${state}`; elements.connectionBadge.textContent = text; }
function showPage(name) { document.querySelectorAll(".page").forEach(p => p.classList.toggle("active", p.id === `page-${name}`)); document.querySelectorAll(".tab-button").forEach(b => b.classList.toggle("active", b.dataset.page === name)); }
function updateMqttFieldState() { const enabled = elements.mqttEnabled.checked; elements.mqttFields.querySelectorAll("input").forEach(i => i.disabled = !enabled); elements.mqttHost.required = enabled; }
function setSaveMessage(message, type = "") { elements.saveMessage.textContent = message; elements.saveMessage.className = `form-message ${type}`.trim(); }
function setSlotMessage(message, type = "") { elements.slotMessage.textContent = message; elements.slotMessage.className = `form-message ${type}`.trim(); }
function formatAge(ms) { if (!Number.isFinite(ms)) return "-"; if (ms < 1000) return "just now"; return `${Math.floor(ms / 1000)} s ago`; }
function formatUptime(seconds) {
  const total = Math.max(0, Number(seconds) || 0);
  const days = Math.floor(total / 86400);
  const hours = Math.floor((total % 86400) / 3600);
  const minutes = Math.floor((total % 3600) / 60);
  return days > 0 ? `${days} d ${hours} h` : `${hours} h ${minutes} min`;
}
function fingerprintHex(value) { return Number(value || 0).toString(16).toUpperCase().padStart(8, "0"); }
async function requestJson(url, options = {}) {
  const controller = new AbortController();
  const timeout = window.setTimeout(() => controller.abort(), 8000);
  try {
    const response = await fetch(url, { cache: "no-store", signal: controller.signal, ...options });
    const text = await response.text();
    let data;
    try {
      data = JSON.parse(text);
    } catch {
      const preview = text.slice(0, 180).replace(/\s+/g, " ");
      throw new Error(`Invalid server response (HTTP ${response.status}, ${text.length} bytes): ${preview || "empty body"}`);
    }
    if (!response.ok) throw new Error(data.message || `HTTP ${response.status}`);
    return data;
  } catch (error) {
    if (error.name === "AbortError") throw new Error("The device did not answer within 8 seconds.");
    throw error;
  } finally {
    window.clearTimeout(timeout);
  }
}
async function postJson(url, payload) { return requestJson(url, { method: "POST", headers: { "Content-Type": "application/json" }, body: JSON.stringify(payload) }); }

async function loadRadioStatus() {
  try {
    const d = await requestJson("/api/radio");
    const frequency = Number(d.frequency_mhz);
    const rssi = Number(d.rssi_dbm);
    elements.radioChip.textContent = d.chip || "-";
    elements.radioMode.textContent = d.mode || "-";
    elements.radioFrequency.textContent = Number.isFinite(frequency) ? `${frequency.toFixed(3)} MHz` : "-";
    elements.radioRssi.textContent = Number.isFinite(rssi) ? `${rssi.toFixed(1)} dBm` : "-";
  } catch (error) {
    elements.radioMode.textContent = "API error";
    elements.radioFrequency.textContent = "-";
    elements.radioRssi.textContent = "-";
  }
}
async function loadStatus() {
  elements.refreshStatusButton.disabled = true;
  setConnectionBadge("loading", "Connecting");
  let lastError;
  for (let attempt = 0; attempt < 2; attempt++) {
    try {
      const d = await requestJson("/api/status");
      elements.status.textContent = "Online";
      setGauge(elements.core0Gauge, elements.core0Load, d.core0_load_percent);
      setGauge(elements.core1Gauge, elements.core1Load, d.core1_load_percent);
      setGauge(elements.psramGauge, elements.psramLoad, d.psram_used_percent);
      setGauge(elements.heapGauge, elements.heapLoad, d.heap_used_percent);
      renderMemoryDiagnostics(d);
      elements.firmware.textContent = d.version || "-"; if (elements.uiVersion) elements.uiVersion.textContent = `v${String(d.version || "0.6.0a").replace("-alpha","")}`;
      elements.ip.textContent = d.ip || "-";
      elements.wifiMode.textContent = d.wifi_mode || "-";
      elements.mqttStatus.textContent = d.mqtt_state || (d.mqtt_enabled ? "Disconnected" : "Disabled");
      elements.freeHeap.textContent = `${Number(d.free_heap || 0).toLocaleString()} B`;
      elements.heapFragmentation.textContent = `${Number(d.heap_fragmentation_percent || 0)}%`;
      elements.uptime.textContent = formatUptime(d.uptime_seconds);
      setConnectionBadge("online", "Online");
      loadRadioStatus();
      elements.refreshStatusButton.disabled = false;
      return;
    } catch (error) {
      lastError = error;
      if (attempt === 0) await new Promise(resolve => setTimeout(resolve, 350));
    }
  }
  console.error(lastError);
  elements.status.textContent = "Offline";
  setConnectionBadge("error", "Offline");
  elements.refreshStatusButton.disabled = false;
}
async function loadRawFrame() { try { const d = await requestJson("/api/radio/raw"); if (!d.available) return; elements.rawPulseCount.textContent = `${d.pulse_count} pulses`; elements.rawDuration.textContent = `${(Number(d.duration_us) / 1000).toFixed(2)} ms`; elements.rawRssi.textContent = `${Number(d.rssi_dbm).toFixed(1)} dBm`; elements.rawSequence.textContent = `RAW frame #${d.sequence}`; elements.rawAge.textContent = formatAge(Number(d.age_ms)); const raw = Array.isArray(d.raw) ? d.raw : []; const preview = raw.slice(0, 160).join(", "); elements.rawPreview.textContent = raw.length > 160 ? `${preview}\n\n… ${raw.length - 160} more pulses` : preview; } catch (e) { console.error(e); } }


let analyzerSettingsSaveTimer = null;
let analyzerSettingsEditing = false;
let analyzerRequestActive = false;
let analyzerLiveRequestActive = false;
let analyzerRenderedSequence = -1;
let analyzerRenderedCandidateSequence = -1;
let analyzerLastFullFetchMs = 0;
let analyzerPendingFullRefresh = false;
let analyzerCandidateRefreshTimer = null;
const ANALYZER_MIN_FULL_REFRESH_MS = 250;
const ANALYZER_CANDIDATE_DEBOUNCE_MS = 350;


function renderAnalyzerControls() {
  const rssi = Number(elements.analyzerRssiThreshold?.value);
  const pulses = Number(elements.analyzerMinPulses?.value);
  const duration = Number(elements.analyzerMinDuration?.value);
  const similarity = Number(elements.analyzerSimilarity?.value);
  const occurrences = Number(elements.analyzerOccurrences?.value);
  const alternation = Number(elements.analyzerAlternation?.value);
  if (Number.isFinite(rssi)) elements.analyzerRssiThresholdValue.textContent = `${rssi} dBm`;
  if (Number.isFinite(pulses)) elements.analyzerMinPulsesValue.textContent = String(pulses);
  if (Number.isFinite(duration)) elements.analyzerMinDurationValue.textContent = `${(duration / 1000).toFixed(1)} ms`;
  if (Number.isFinite(similarity)) elements.analyzerSimilarityValue.textContent = `${similarity}%`;
  if (Number.isFinite(occurrences)) elements.analyzerOccurrencesValue.textContent = String(occurrences);
  if (Number.isFinite(alternation)) elements.analyzerAlternationValue.textContent = `${alternation}%`;
  const developerOn = !!elements.analyzerDeveloperMode?.checked;
  if (elements.developerModeState) elements.developerModeState.textContent = developerOn ? "ON" : "OFF";
  if (elements.developerExclusiveWarning) elements.developerExclusiveWarning.hidden = true;
  if (elements.candidateDeveloperMetrics) elements.candidateDeveloperMetrics.hidden = !developerOn;
  document.querySelectorAll(".developer-setting, .developer-only").forEach(el => { el.hidden = !developerOn; });
}

async function saveAnalyzerSettings() {
  elements.analyzerRssiSaveState.textContent = "Saving...";
  elements.analyzerRssiSaveState.className = "form-message";
  try {
    await postJson("/api/analyzer/settings", {
      min_rssi: Number(elements.analyzerRssiThreshold.value),
      min_pulse_count: Number(elements.analyzerMinPulses.value),
      min_duration_us: Number(elements.analyzerMinDuration.value),
      similarity: Number(elements.analyzerSimilarity.value),
      occurrences: Number(elements.analyzerOccurrences.value),
      show_rejected: elements.analyzerShowRejected.checked,
      freeze_candidate: elements.analyzerFreezeCandidate.checked,
      alternation_tolerance: Number(elements.analyzerAlternation.value),
      developer_mode: elements.analyzerDeveloperMode.checked
    });
    elements.analyzerRssiSaveState.textContent = "Saved";
    elements.analyzerRssiSaveState.className = "form-message success";
  } catch (error) {
    elements.analyzerRssiSaveState.textContent = error.message;
    elements.analyzerRssiSaveState.className = "form-message error";
  }
}


async function toggleAnalyzerDeveloperMode() {
  analyzerSettingsEditing = true;
  renderAnalyzerControls();
  elements.analyzerRssiSaveState.textContent = "Switching mode...";
  elements.analyzerRssiSaveState.className = "form-message";
  window.clearTimeout(analyzerSettingsSaveTimer);
  await saveAnalyzerSettings();
  analyzerSettingsEditing = false;
  await loadAnalyzer();
}

function queueAnalyzerSettingsSave() {
  analyzerSettingsEditing = true;
  renderAnalyzerControls();
  elements.analyzerRssiSaveState.textContent = "Release to save";
  window.clearTimeout(analyzerSettingsSaveTimer);
  analyzerSettingsSaveTimer = window.setTimeout(async () => {
    await saveAnalyzerSettings();
    analyzerSettingsEditing = false;
  }, 450);
}

async function loadAnalyzerLatest() {
  if (analyzerRequestActive) {
    analyzerPendingFullRefresh = true;
    return;
  }

  const now = Date.now();
  const wait = ANALYZER_MIN_FULL_REFRESH_MS - (now - analyzerLastFullFetchMs);
  if (wait > 0) {
    analyzerPendingFullRefresh = true;
    window.setTimeout(() => {
      if (analyzerPendingFullRefresh) {
        analyzerPendingFullRefresh = false;
        loadAnalyzerLatest();
      }
    }, wait);
    return;
  }

  analyzerPendingFullRefresh = false;
  analyzerLastFullFetchMs = Date.now();
  await loadAnalyzer();

  // If more RF data arrived while the full document was in flight, do one
  // additional fetch for the newest snapshot. Intermediate states are dropped.
  if (analyzerPendingFullRefresh) {
    analyzerPendingFullRefresh = false;
    loadAnalyzerLatest();
  }
}

function scheduleCandidateAnalyzerRefresh() {
  if (analyzerCandidateRefreshTimer) {
    window.clearTimeout(analyzerCandidateRefreshTimer);
  }
  analyzerCandidateRefreshTimer = window.setTimeout(() => {
    analyzerCandidateRefreshTimer = null;
    loadAnalyzerLatest();
  }, ANALYZER_CANDIDATE_DEBOUNCE_MS);
}

async function loadAnalyzerFast() {
  if (!elements.analyzerSequence || analyzerLiveRequestActive || analyzerRequestActive) return;
  analyzerLiveRequestActive = true;
  try {
    const d = await requestJson("/api/analyzer/live");
    if (!d.enabled) return;

    if (elements.analyzerPeakRssi && Number(d.peak_rssi_dbm) > -127) {
      elements.analyzerPeakRssi.textContent = `${Number(d.peak_rssi_dbm).toFixed(1)} dBm`;
    }
    if (elements.analyzerWeakRssi) {
      elements.analyzerWeakRssi.textContent = d.weak_rssi_frames || 0;
    }

    const seq = Number(d.sequence || 0);
    const candidateSeq = Number(d.candidate_sequence || 0);

    if (seq !== analyzerRenderedSequence) {
      // Accepted / Analyzer-result frames are user-visible events: refresh ASAP.
      analyzerPendingFullRefresh = true;
      loadAnalyzerLatest();
    } else if (candidateSeq !== analyzerRenderedCandidateSequence) {
      // Rejected/noise candidates can arrive continuously. Coalesce them and
      // render only the newest candidate instead of building an HTTP backlog.
      scheduleCandidateAnalyzerRefresh();
    } else {
      if (d.available && elements.analyzerAge) {
        elements.analyzerAge.textContent =
          `${Math.round(Number(d.age_ms || 0) / 1000)} s ago`;
      }
      if (d.candidate_available && elements.candidateAge) {
        elements.candidateAge.textContent =
          formatAge(Number(d.candidate_age_ms || 0));
      }
    }
  } catch (_) {
  } finally {
    analyzerLiveRequestActive = false;
  }
}

async function loadAnalyzer() {
  if (!elements.analyzerSequence || analyzerRequestActive) return;
  analyzerRequestActive = true;
  try {
    const d = await requestJson("/api/analyzer");
    if (d.analyzer_disabled) {
      if (!analyzerSettingsEditing) {
        elements.analyzerDeveloperMode.checked = false;
        renderAnalyzerControls();
      }
      elements.analyzerBadge.textContent = "STANDBY";
      elements.analyzerBadge.className = "badge badge-loading";
      elements.analyzerSequence.textContent = "Analyzer disabled";
      elements.analyzerDetailTitle.textContent = d.status || "Analyzer disabled - gateway remains active";
      elements.analyzerFrequency.textContent = Number.isFinite(Number(d.frequency_mhz)) ? `${Number(d.frequency_mhz).toFixed(3)} MHz` : "-";
      elements.analyzerCurrentRssi.textContent = Number.isFinite(Number(d.current_rssi_dbm)) ? `${Number(d.current_rssi_dbm).toFixed(1)} dBm` : "-";
      elements.analyzerEncoding.textContent = "Enable RF Analyzer for full diagnostics";
      if (elements.analyzerModeMessage) elements.analyzerModeMessage.innerHTML = "<strong>RF Analyzer is disabled.</strong> The gateway remains fully operational. Enable RF Analyzer for live RF diagnostics; on ESP32-S3 it runs in non-exclusive mode alongside RX Slots, MQTT and Home Assistant.";
      return;
    }
    if (d.low_memory) {
      elements.analyzerBadge.textContent = "PAUSED";
      elements.analyzerBadge.className = "badge badge-warning";
      elements.analyzerDetailTitle.textContent = d.status || "Analyzer paused: low memory";
      elements.analyzerEncoding.textContent = "Memory protection active";
      return;
    }
    elements.analyzerCandidates.textContent = d.raw_candidates || 0;
    elements.analyzerAccepted.textContent = d.accepted_frames || 0;
    elements.analyzerRejected.textContent = d.rejected_frames || 0;
    elements.analyzerDecoded.textContent = d.decoded_frames || 0;
    elements.analyzerUnknown.textContent = d.unknown_frames || 0;
    elements.analyzerWeakRssi.textContent = d.weak_rssi_frames || 0;
    elements.analyzerCurrentRssi.textContent = Number.isFinite(Number(d.current_rssi_dbm)) ? `${Number(d.current_rssi_dbm).toFixed(1)} dBm` : "-";
    elements.analyzerPeakRssi.textContent = Number(d.peak_rssi_dbm) > -127 ? `${Number(d.peak_rssi_dbm).toFixed(1)} dBm` : "-";
    if (!analyzerSettingsEditing) {
      if (Number.isFinite(Number(d.analyzer_min_rssi))) elements.analyzerRssiThreshold.value = String(d.analyzer_min_rssi);
      if (Number.isFinite(Number(d.analyzer_min_pulse_count))) elements.analyzerMinPulses.value = String(d.analyzer_min_pulse_count);
      if (Number.isFinite(Number(d.analyzer_min_duration_us))) elements.analyzerMinDuration.value = String(d.analyzer_min_duration_us);
      if (Number.isFinite(Number(d.analyzer_similarity))) elements.analyzerSimilarity.value = String(d.analyzer_similarity);
      if (Number.isFinite(Number(d.analyzer_occurrences))) elements.analyzerOccurrences.value = String(d.analyzer_occurrences);
      elements.analyzerShowRejected.checked = !!d.analyzer_show_rejected;
      elements.analyzerFreezeCandidate.checked = !!d.analyzer_freeze_candidate;
      if (Number.isFinite(Number(d.analyzer_alternation_tolerance))) elements.analyzerAlternation.value = String(d.analyzer_alternation_tolerance);
      elements.analyzerDeveloperMode.checked = !!d.analyzer_developer_mode;
      renderAnalyzerControls();
    }
    analyzerRenderedSequence = Number(d.sequence || 0);
    const c = d.last_candidate || {};
    analyzerRenderedCandidateSequence = Number(c.sequence || 0);
    if (c.available) {
      elements.candidateSequence.textContent = `Candidate #${c.sequence}`;
      elements.candidateAge.textContent = formatAge(Number(c.age_ms));
      elements.candidateRssi.textContent = `${Number(c.rssi_dbm).toFixed(1)} dBm`;
      elements.candidatePulseCount.textContent = `${c.pulse_count || 0} pulses`;
      elements.candidateDuration.textContent = `${(Number(c.duration_us || 0) / 1000).toFixed(2)} ms`;
      elements.candidateRejectReason.textContent = c.reject_reason || "-";
      elements.candidateAlternation.textContent = `${c.alternation_ratio || 0}%`;
      elements.candidateSamePairs.textContent = String(c.same_sign_pairs || 0);
      elements.candidateLongestRun.textContent = String(c.longest_same_sign_run || 0);
      elements.candidateNormalizedCount.textContent = `${c.normalized_pulse_count || 0} pulses`;
      const normalizedRaw = Array.isArray(c.normalized_pulses_us) ? c.normalized_pulses_us : [];
      elements.candidateNormalizedRaw.textContent = `Normalized signed pulse sequence (µs):\n${normalizedRaw.join(", ")}`;
      const candidateRaw = Array.isArray(c.raw_pulses_us) ? c.raw_pulses_us : [];
      elements.candidateRaw.textContent = `Frequency: ${Number(c.frequency_mhz).toFixed(3)} MHz
Pulse min / max: ${c.min_pulse_us || 0} / ${c.max_pulse_us || 0} µs
RAW signed pulse sequence (µs):
${candidateRaw.join(", ")}${c.raw_truncated ? `
… preview truncated after ${candidateRaw.length} pulses` : ""}`;
    }
    if (!d.available) return;
    elements.analyzerSequence.textContent = `Frame #${d.sequence}`;
    elements.analyzerBadge.textContent = d.status || "ACTIVE";
    elements.analyzerBadge.className = `badge ${d.accepted ? "badge-online" : "badge-loading"}`;
    if (elements.analyzerModeMessage) elements.analyzerModeMessage.innerHTML = "<strong>RF Analyzer is running.</strong> Capturing RF traffic in the background. On ESP32-S3, Analyzer runs in non-exclusive mode while the gateway remains fully operational.";
    elements.analyzerFrequency.textContent = `${Number(d.frequency_mhz).toFixed(3)} MHz`;
    elements.analyzerRssi.textContent = `${Number(d.rssi_dbm).toFixed(1)} dBm`;
    elements.analyzerProtocol.textContent = d.protocol || "Unknown";
    elements.analyzerEncoding.textContent = d.encoding || "Unknown";
    elements.analyzerBits.textContent = d.symbol_count || "-";
    elements.analyzerBasePulse.textContent = d.base_pulse_us ? `${d.base_pulse_us} µs` : (d.shortest_class_us ? `~${d.shortest_class_us} µs` : "-");
    elements.analyzerFrameCount.textContent = d.frame_count || "-";
    elements.analyzerQuality.textContent = d.quality ? `${d.quality}%` : "-";
    elements.analyzerPulseCount.textContent = `${d.pulse_count || 0} pulses`;
    elements.analyzerDuration.textContent = `${(Number(d.duration_us || 0) / 1000).toFixed(2)} ms`;
    elements.analyzerCode.textContent = d.code_hex ? `0x${String(d.code_hex).toUpperCase()}` : "-";
    elements.analyzerClasses.textContent = Array.isArray(d.pulse_classes_us) && d.pulse_classes_us.length ? d.pulse_classes_us.map(v => `${v} µs`).join(", ") : "-";
    elements.analyzerAge.textContent = `${Math.round(Number(d.age_ms || 0) / 1000)} s ago`;
    const reason = d.accepted ? "accepted" : (d.reject_reason || "rejected");
    elements.analyzerDetailTitle.textContent = `${d.protocol || "Unknown"} · ${reason}`;
    const raw = Array.isArray(d.raw_pulses_us) ? d.raw_pulses_us : [];
    const rawText = raw.length ? raw.join(", ") + (d.raw_truncated ? `\n… preview truncated after ${raw.length} pulses` : "") : "No RAW pulse data.";
    const decodedText = d.bitstream ? `Decoded bitstream: ${d.bitstream}\n\n` : "";
    const structuredText = d.structured_signal ? `Structured signal: yes\nOccurrences: ${d.occurrences || 0}\nSimilarity: ${d.similarity || 0}%\n\n` : "";
    elements.analyzerBitstream.textContent = `${decodedText}${structuredText}Protocol: ${d.protocol || "Unknown"}
Encoding guess: ${d.encoding || "Unknown"}
Status: ${reason}
Frequency: ${Number(d.frequency_mhz).toFixed(3)} MHz
RSSI: ${Number(d.rssi_dbm).toFixed(1)} dBm
Pulse count: ${d.pulse_count || 0}
Duration: ${d.duration_us || 0} µs
Pulse min / avg / max: ${d.min_pulse_us || 0} / ${d.average_pulse_us || 0} / ${d.max_pulse_us || 0} µs
Pulse classes: ${elements.analyzerClasses.textContent}
Class ratio: ${Number(d.class_ratio || 0).toFixed(2)}
Base pulse estimate: ${d.shortest_class_us || 0} µs
RAW signed pulse sequence (µs):
${rawText}`;
  } catch (e) {
    elements.analyzerDetailTitle.textContent = "Analyzer API error";
    elements.analyzerBitstream.textContent = e.message;
  } finally {
    analyzerRequestActive = false;
  }
}

function renderLearnState(d) {
  const state = d.state || "IDLE";
  elements.learnState.textContent = state; elements.learnBadge.className = "badge badge-loading";
  elements.learnStartButton.disabled = state === "WAITING_FOR_SIGNAL";
  elements.learnAcceptButton.disabled = state !== "PREVIEW_READY";
  elements.learnSaveButton.disabled = !(state === "PREVIEW_READY" || state === "ACCEPTED_RAM");
  elements.learnTestSendButton.disabled = !(state === "PREVIEW_READY" || state === "ACCEPTED_RAM");
  elements.learnDiscardButton.disabled = state === "IDLE";
  if (state === "WAITING_FOR_SIGNAL") { elements.learnBadge.textContent = "Listening"; elements.learnMessage.textContent = "Press the remote button once. The next valid frame becomes the preview."; }
  else if (state === "PREVIEW_READY") { elements.learnBadge.className = "badge badge-online"; elements.learnBadge.textContent = "Preview ready"; elements.learnMessage.textContent = "Test the capture, then save it to a permanent slot."; }
  else if (state === "ACCEPTED_RAM") { elements.learnBadge.className = "badge badge-online"; elements.learnBadge.textContent = "Accepted"; elements.learnMessage.textContent = "Capture accepted in RAM and ready to save."; }
  else { elements.learnBadge.textContent = "Idle"; elements.learnMessage.textContent = "Start learning, then press the remote button once."; }
  elements.learnPulseCount.textContent = d.available ? `${d.pulse_count} pulses` : "-";
  elements.learnDuration.textContent = d.available ? `${(Number(d.duration_us) / 1000).toFixed(2)} ms` : "-";
  elements.learnRssi.textContent = d.available ? `${Number(d.rssi_dbm).toFixed(1)} dBm` : "-";
  elements.learnNoiseFloor.textContent = Number.isFinite(Number(d.noise_floor_dbm)) ? `${Number(d.noise_floor_dbm).toFixed(1)} dBm` : "-";
  elements.learnRejected.textContent = d.rejected_during_learn || 0; elements.learnRejectReason.textContent = d.last_reject_reason || "-";
}
async function loadLearnStatus() { try { const d = await requestJson("/api/radio/learn"); renderLearnState(d); if (d.available) await loadLearnRaw(); else { elements.learnPreviewTitle.textContent = "No preview"; elements.learnRawPreview.textContent = "The captured signal will appear here before it is accepted."; } } catch (e) { elements.learnMessage.textContent = e.message; } }
async function loadLearnRaw() { try { const d = await requestJson("/api/radio/learn/raw"); const raw = Array.isArray(d.raw) ? d.raw : []; const preview = raw.slice(0, 240).join(", "); elements.learnPreviewTitle.textContent = `${d.pulse_count} pulses captured`; elements.learnRawPreview.textContent = raw.length > 240 ? `${preview}\n\n… ${raw.length - 240} more pulses` : preview; } catch (e) { console.error(e); } }
async function learnAction(path) { try { const d = await requestJson(path, { method: "POST" }); elements.learnMessage.textContent = d.message; await loadLearnStatus(); } catch (e) { elements.learnMessage.textContent = e.message; } }
let slotSaveSnapshot = [];

function closeSlotSaveModal() {
  elements.slotSaveModal.classList.remove("open");
  elements.slotSaveModal.setAttribute("aria-hidden", "true");
}

function updateSlotSaveSelection() {
  const slotId = Number(elements.slotSaveSelect.value);
  const slot = slotSaveSnapshot.find(item => Number(item.id) === slotId);
  if (!slot) return;
  elements.slotSaveName.value = slot.used ? (slot.name || `RF Slot ${slotId}`) : `RF Slot ${slotId}`;
  elements.slotSaveWarning.textContent = slot.used
    ? `Slot ${slotId} is occupied. Saving will overwrite “${slot.name || `RF Slot ${slotId}`}”.`
    : `Slot ${slotId} is empty and safe to use.`;
  elements.slotSaveWarning.className = `slot-save-warning ${slot.used ? "warning" : "safe"}`;
  elements.slotSaveConfirm.textContent = slot.used ? "Overwrite slot" : "Save to slot";
}

async function saveLearnToSlot() {
  try {
    const data = await requestJson("/api/slots");
    slotSaveSnapshot = Array.isArray(data.slots) ? data.slots : [];
    elements.slotSaveSelect.replaceChildren();

    const ordered = [
      ...slotSaveSnapshot.filter(slot => !slot.used),
      ...slotSaveSnapshot.filter(slot => slot.used)
    ];

    ordered.forEach(slot => {
      const option = document.createElement("option");
      option.value = String(slot.id);
      option.textContent = slot.used
        ? `Slot ${slot.id} — ${slot.name || `RF Slot ${slot.id}`} (occupied)`
        : `Slot ${slot.id} — Empty`;
      elements.slotSaveSelect.append(option);
    });

    const firstEmpty = ordered.find(slot => !slot.used);
    if (firstEmpty) elements.slotSaveSelect.value = String(firstEmpty.id);
    else if (ordered.length) elements.slotSaveSelect.value = String(ordered[0].id);

    updateSlotSaveSelection();
    elements.slotSaveModal.classList.add("open");
    elements.slotSaveModal.setAttribute("aria-hidden", "false");
  } catch (e) {
    elements.learnMessage.textContent = e.message;
  }
}

async function confirmSlotSave() {
  const slotId = Number(elements.slotSaveSelect.value);
  const slot = slotSaveSnapshot.find(item => Number(item.id) === slotId);
  if (!slot) return;

  if (slot.used && !window.confirm(`Slot ${slotId} already contains “${slot.name || `RF Slot ${slotId}`}”. Overwrite it?`)) return;

  const defaultName = `RF Slot ${slotId}`;
  const name = elements.slotSaveName.value.trim() || defaultName;
  elements.slotSaveConfirm.disabled = true;
  try {
    const d = await postJson("/api/slots/save", { slot: slotId, name });
    closeSlotSaveModal();
    elements.learnMessage.textContent = `${d.message}. Fingerprint: ${fingerprintHex(d.fingerprint)}`;
    await loadSlots();
  } catch (e) {
    elements.learnMessage.textContent = e.message;
  } finally {
    elements.slotSaveConfirm.disabled = false;
  }
}

function slotCard(slot) {
  const card = document.createElement("article"); card.className = `card slot-card${slot.used ? "" : " empty"}`;
  const title = document.createElement("div"); title.className = "slot-title";
  const heading = document.createElement("div"); heading.innerHTML = `<span class="slot-number">Slot ${slot.id}</span><h3></h3>`; heading.querySelector("h3").textContent = slot.name || `RF Slot ${slot.id}`;
  const badge = document.createElement("span"); badge.className = `badge ${slot.used ? "badge-online" : "badge-loading"}`; badge.textContent = slot.used ? "Saved" : "Empty";
  title.append(heading, badge); card.append(title);
  if (slot.used) {
    const meta = document.createElement("div"); meta.className = "slot-meta";
    meta.innerHTML = `<span>Pulses<strong>${slot.pulse_count}</strong></span><span>Duration<strong>${(Number(slot.duration_us) / 1000).toFixed(2)} ms</strong></span><span>Frequency<strong>${Number(slot.frequency_mhz).toFixed(3)} MHz</strong></span><span>Fingerprint<strong>${fingerprintHex(slot.fingerprint)}</strong></span>`;
    card.append(meta);
    const input = document.createElement("input"); input.className = "slot-name-input"; input.maxLength = 32; input.value = slot.name;
    const actions = document.createElement("div"); actions.className = "slot-actions";
    const send = document.createElement("button"); send.className = "button button-primary"; send.textContent = "Send"; send.onclick = () => slotAction("send", slot.id);
    const rename = document.createElement("button"); rename.className = "button button-secondary"; rename.textContent = "Rename"; rename.onclick = () => slotAction("rename", slot.id, input.value);
    const del = document.createElement("button"); del.className = "button button-danger"; del.textContent = "Delete"; del.onclick = () => { if (window.confirm(`Delete slot ${slot.id}?`)) slotAction("delete", slot.id); };
    actions.append(send, rename, del); card.append(input, actions);
  } else {
    const text = document.createElement("p"); text.className = "muted"; text.textContent = "Learn a signal, then use Save to Slot."; card.append(text);
  }
  return card;
}
async function loadSlots() { if (!elements.slotGrid) return; elements.refreshSlotsButton.disabled = true; setSlotMessage("Loading..."); try { const d = await requestJson("/api/slots"); elements.slotUsage.textContent = `${d.used_count || 0} of ${d.count || 30} slots used`; elements.slotGrid.replaceChildren(...(d.slots || []).map(slotCard)); setSlotMessage(""); } catch (e) { setSlotMessage(e.message, "error"); } finally { elements.refreshSlotsButton.disabled = false; } }
async function slotAction(action, slot, name = "") { setSlotMessage(`${action}...`); try { const payload = { slot }; if (action === "rename") payload.name = name.trim(); const d = await postJson(`/api/slots/${action}`, payload); setSlotMessage(d.message, "success"); if (action !== "send") await loadSlots(); } catch (e) { setSlotMessage(e.message, "error"); } }


function setRxSlotMessage(message, type = "") { elements.rxSlotMessage.textContent = message; elements.rxSlotMessage.className = `form-message ${type}`.trim(); }
let rxLearnPollTimer = null;
let rxLearnDeadline = 0;
let rxLearningSlot = 0;

function stopRxLearnPolling() {
  if (rxLearnPollTimer) window.clearTimeout(rxLearnPollTimer);
  rxLearnPollTimer = null;
  rxLearnDeadline = 0;
  rxLearningSlot = 0;
}

function rxSlotCard(slot, learningSlot = 0) {
  const card=document.createElement("article"); card.className=`card slot-card${slot.used?"":" empty"}${slot.id===learningSlot?" rx-learning":""}`;
  const title=document.createElement("div"); title.className="slot-title";
  const heading=document.createElement("div"); heading.innerHTML=`<span class="slot-number">RX Slot ${slot.id}</span><h3></h3>`; heading.querySelector("h3").textContent=slot.name||`RX Slot ${slot.id}`;
  const badge=document.createElement("span"); badge.className=`badge ${slot.id===learningSlot?"badge-loading":(slot.used&&slot.enabled?"badge-online":"badge-loading")}`; badge.textContent=slot.id===learningSlot?"Waiting":(slot.used?(slot.enabled?"Active":"Disabled"):"Empty"); title.append(heading,badge);card.append(title);
  if(slot.used){
    const meta=document.createElement("div");meta.className="slot-meta";meta.innerHTML=`<span>Protocol<strong>${slot.protocol||"Unknown"}</strong></span><span>Code<strong>${slot.code||"—"}</strong></span><span>Symbols<strong>${slot.symbol_count||0}</strong></span><span>Matches<strong>${slot.match_count||0}</strong></span><span>Last quality<strong>${Number(slot.last_quality||0).toFixed(0)}%</strong></span><span>Last RSSI<strong>${Number(slot.last_rssi||-127).toFixed(1)} dBm</strong></span>`;card.append(meta);
    const input=document.createElement("input");input.className="slot-name-input";input.maxLength=32;input.value=slot.name;
    const actions=document.createElement("div");actions.className="slot-actions";
    const learn=document.createElement("button");learn.className="button button-primary";learn.textContent="Capture again";learn.disabled=Boolean(learningSlot);learn.onclick=()=>rxSlotAction("learn",slot.id,input.value, false, learn);
    const rename=document.createElement("button");rename.className="button button-secondary";rename.textContent="Rename";rename.disabled=Boolean(learningSlot);rename.onclick=()=>rxSlotAction("rename",slot.id,input.value);
    const enable=document.createElement("button");enable.className="button button-secondary";enable.textContent=slot.enabled?"Disable":"Enable";enable.disabled=Boolean(learningSlot);enable.onclick=()=>rxSlotAction("enable",slot.id,"",!slot.enabled);
    const del=document.createElement("button");del.className="button button-danger";del.textContent="Delete";del.disabled=Boolean(learningSlot);del.onclick=()=>{if(confirm(`Delete RX slot ${slot.id}?`))rxSlotAction("delete",slot.id);};actions.append(learn,rename,enable,del);card.append(input,actions);
  } else {
    const input=document.createElement("input");input.className="slot-name-input";input.maxLength=32;input.value=`RX Slot ${slot.id}`;
    const learn=document.createElement("button");learn.className="button button-primary";learn.textContent=slot.id===learningSlot?"Waiting for signal...":"Capture button";learn.disabled=Boolean(learningSlot);learn.onclick=()=>rxSlotAction("learn",slot.id,input.value, false, learn);card.append(input,learn);
  }
  return card;
}

function renderRxLearnRssi(value) {
  const rssi = Math.max(-100, Math.min(-20, Number(value) || -75));
  if (elements.rxLearnRssiThreshold) elements.rxLearnRssiThreshold.value = String(rssi);
  if (elements.rxLearnRssiValue) elements.rxLearnRssiValue.textContent = `${rssi} dBm`;
}

let rxLearnRssiSaveTimer = null;
function queueRxLearnRssiSave() {
  const value = Number(elements.rxLearnRssiThreshold?.value || -75);
  renderRxLearnRssi(value);
  if (elements.rxLearnRssiSaveState) {
    elements.rxLearnRssiSaveState.textContent = "Saving...";
    elements.rxLearnRssiSaveState.className = "form-message";
  }
  if (rxLearnRssiSaveTimer) window.clearTimeout(rxLearnRssiSaveTimer);
  rxLearnRssiSaveTimer = window.setTimeout(async () => {
    try {
      const d = await postJson("/api/rxslots/learn-rssi", { min_rssi: value });
      renderRxLearnRssi(d.min_rssi);
      if (elements.rxLearnRssiSaveState) {
        elements.rxLearnRssiSaveState.textContent = "Saved automatically";
        elements.rxLearnRssiSaveState.className = "form-message success";
      }
    } catch (e) {
      if (elements.rxLearnRssiSaveState) {
        elements.rxLearnRssiSaveState.textContent = e.message;
        elements.rxLearnRssiSaveState.className = "form-message error";
      }
    }
  }, 250);
}

async function loadRxSlots({silent=false}={}) {
  if(!elements.rxSlotGrid)return null;
  if(!silent) elements.refreshRxSlotsButton.disabled=true;
  if(!silent) setRxSlotMessage("Loading...");
  try {
    const d=await requestJson("/api/rxslots");
    const learningSlot=Number(d.learning_slot||0);
    renderRxLearnRssi(d.learn_min_rssi);
    elements.rxSlotUsage.textContent=`${d.used_count||0} of ${d.count||10} RX slots used${learningSlot?` — capturing into slot ${learningSlot}`:""}`;
    elements.rxSlotGrid.replaceChildren(...(d.slots||[]).map(slot=>rxSlotCard(slot,learningSlot)));
    if(d.learn_state==="waiting_for_signal") {
      const weak=Number(d.weak_rejected||0);
      if(weak>0) setRxSlotMessage(`Waiting for a stronger signal... ${weak} weak capture${weak===1?"":"s"} ignored; last ${Number(d.last_weak_rssi||-127).toFixed(1)} dBm, minimum ${Number(d.learn_min_rssi||-75)} dBm.`);
      else setRxSlotMessage("Waiting for a supported fixed-code RF signal...");
    }
    else if(d.learn_state==="unsupported_protocol") setRxSlotMessage("Unsupported or ambiguous protocol. No RX slot was saved.","error");
    else if(d.learn_state==="duplicate_code") setRxSlotMessage("This RF code is already assigned to another RX slot.","error");
    else if(d.learn_state==="saved") setRxSlotMessage("RX button captured and saved. Home Assistant discovery update queued.","success");
    else if(d.learn_state==="save_error") setRxSlotMessage("The RF signal was received, but saving failed.","error");
    else if(d.learn_state==="radio_error") setRxSlotMessage("Radio could not re-arm after rejecting a weak Learn signal.","error");
    else if(!silent) setRxSlotMessage("");
    return d;
  } catch(e) { setRxSlotMessage(e.message,"error"); return null; }
  finally { if(!silent) elements.refreshRxSlotsButton.disabled=false; }
}

async function pollRxLearn() {
  const data = await loadRxSlots({silent:true});
  if (!data) { stopRxLearnPolling(); return; }
  if (data.learn_state === "waiting_for_signal" && Number(data.learning_slot||0) > 0) {
    if (Date.now() >= rxLearnDeadline) { setRxSlotMessage("Capture is still waiting. Press Refresh to check later, or start again after reboot.","error"); stopRxLearnPolling(); return; }
    rxLearnPollTimer = window.setTimeout(pollRxLearn, 650);
    return;
  }
  if (data.learn_state === "saved") {
    setRxSlotMessage(`RX slot ${rxLearningSlot || ""} captured successfully. Home Assistant discovery was republished.`,"success");
    stopRxLearnPolling();
    window.setTimeout(()=>loadRxSlots(),1200);
    return;
  }
  if (data.learn_state === "save_error") setRxSlotMessage("RX capture failed while saving.","error");
  stopRxLearnPolling();
}

async function rxSlotAction(action,slot,name="",enabled=false,button=null) {
  setRxSlotMessage(action==="learn"?"Starting RX capture...":`${action}...`);
  if(button){button.disabled=true;button.classList.add("busy");}
  try {
    const payload={slot};if(action==="learn"||action==="rename")payload.name=name.trim();if(action==="enable")payload.enabled=enabled;
    const d=await postJson(`/api/rxslots/${action}`,payload);
    if(action==="learn") {
      rxLearningSlot=slot; rxLearnDeadline=Date.now()+45000;
      setRxSlotMessage("Waiting for RF signal. Press the remote button once...");
      await loadRxSlots({silent:true});
      rxLearnPollTimer=window.setTimeout(pollRxLearn,500);
    } else {
      setRxSlotMessage(d.message,"success"); await loadRxSlots();
    }
  } catch(e) { setRxSlotMessage(e.message,"error"); stopRxLearnPolling(); }
  finally { if(button){button.classList.remove("busy"); if(action!=="learn")button.disabled=false;} }
}

async function loadConfig() { setSaveMessage("Loading settings..."); elements.saveButton.disabled = true; try { const d = await requestJson("/api/config"); elements.hostname.value = d.hostname || "OpenRF-Platform"; elements.replayCount.value = Number(d.replay_count) >= 1 ? d.replay_count : 1; elements.radioBand.value = String(Number(d.radio_frequency_mhz) === 868 ? 868 : 433); elements.wifiSsid.value = d.wifi_ssid || ""; elements.wifiPassword.value = ""; elements.wifiPasswordState.textContent = d.wifi_password_set ? "A WiFi password is saved. Leave empty to keep it." : "No WiFi password is saved."; elements.mqttEnabled.checked = Boolean(d.mqtt_enabled); elements.mqttHost.value = d.mqtt_host || ""; elements.mqttPort.value = d.mqtt_port || 1883; elements.mqttUser.value = d.mqtt_user || ""; elements.mqttPassword.value = ""; elements.homeAssistantDiscovery.checked = d.home_assistant_discovery !== false; elements.passwordState.textContent = d.mqtt_password_set ? "A password is saved. Leave empty to keep it." : "No MQTT password is saved."; updateMqttFieldState(); setSaveMessage(""); } catch (e) { setSaveMessage(e.message, "error"); } finally { elements.saveButton.disabled = false; } }
async function saveConfig(event) { event.preventDefault(); if (!elements.settingsForm.reportValidity()) return; const payload = { hostname: elements.hostname.value.trim(), wifi_ssid: elements.wifiSsid.value.trim(), wifi_password: elements.wifiPassword.value, replay_count: Number(elements.replayCount.value), radio_frequency_mhz: Number(elements.radioBand.value), mqtt_enabled: elements.mqttEnabled.checked, mqtt_host: elements.mqttHost.value.trim(), mqtt_port: Number(elements.mqttPort.value || 1883), mqtt_user: elements.mqttUser.value.trim(), mqtt_password: elements.mqttPassword.value, home_assistant_discovery: elements.homeAssistantDiscovery.checked }; elements.saveButton.disabled = true; setSaveMessage("Saving..."); try { const r = await postJson("/api/config", payload); elements.wifiPassword.value = ""; elements.mqttPassword.value = ""; setSaveMessage(r.restart_required ? "Configuration saved. Restarting now; reconnect using the device network IP." : (r.message || "Configuration saved"), "success"); } catch (e) { setSaveMessage(e.message, "error"); } finally { elements.saveButton.disabled = false; } }


function uploadFile(url, file, progressElement, messageElement) {
  return new Promise((resolve, reject) => {
    const xhr = new XMLHttpRequest();
    const form = new FormData();
    form.append("file", file, file.name);
    xhr.open("POST", url, true);
    xhr.timeout = 180000;
    xhr.upload.onprogress = event => {
      if (!event.lengthComputable) return;
      const percent = Math.round((event.loaded / event.total) * 100);
      progressElement.style.width = `${percent}%`;
      messageElement.textContent = `Uploading... ${percent}%`;
      messageElement.className = "form-message";
    };
    xhr.onload = () => {
      let data = {};
      try { data = JSON.parse(xhr.responseText || "{}"); } catch { data = { message: xhr.responseText || `HTTP ${xhr.status}` }; }
      if (xhr.status >= 200 && xhr.status < 300) resolve(data);
      else reject(new Error(data.message || `HTTP ${xhr.status}`));
    };
    xhr.onerror = () => reject(new Error("Connection failed during upload."));
    xhr.ontimeout = () => reject(new Error("Upload timed out."));
    xhr.send(form);
  });
}

async function installFirmware() {
  const file = elements.otaFile.files[0];
  if (!file) { elements.otaMessage.textContent = "Select firmware.bin first."; elements.otaMessage.className = "form-message error"; return; }
  if (!file.name.toLowerCase().endsWith(".bin")) { elements.otaMessage.textContent = "Only .bin firmware files are accepted."; elements.otaMessage.className = "form-message error"; return; }
  if (!window.confirm("Install this firmware now? Configuration and RF slots will be preserved.")) return;
  elements.otaUploadButton.disabled = true;
  elements.otaProgress.style.width = "0%";
  try {
    const result = await uploadFile("/api/system/ota", file, elements.otaProgress, elements.otaMessage);
    elements.otaProgress.style.width = "100%";
    elements.otaMessage.textContent = result.message || "Firmware installed. Restarting...";
    elements.otaMessage.className = "form-message success";
  } catch (error) {
    elements.otaMessage.textContent = error.message;
    elements.otaMessage.className = "form-message error";
    elements.otaUploadButton.disabled = false;
  }
}

async function restoreBackup() {
  const file = elements.backupFile.files[0];
  if (!file) { elements.backupMessage.textContent = "Select an .orfbackup file first."; elements.backupMessage.className = "form-message error"; return; }
  if (!window.confirm("Restore this backup? Current configuration and saved TX slots will be replaced.")) return;
  elements.backupRestoreButton.disabled = true;
  elements.backupProgress.style.width = "0%";
  try {
    const result = await uploadFile("/api/system/restore", file, elements.backupProgress, elements.backupMessage);
    elements.backupProgress.style.width = "100%";
    elements.backupMessage.textContent = result.message || "Backup restored. Restarting...";
    elements.backupMessage.className = "form-message success";
  } catch (error) {
    elements.backupMessage.textContent = error.message;
    elements.backupMessage.className = "form-message error";
    elements.backupRestoreButton.disabled = false;
  }
}

document.addEventListener("DOMContentLoaded", () => {
  document.querySelectorAll(".tab-button").forEach(button => button.addEventListener("click", () => { const page = button.dataset.page; if (page !== "rxslots" && rxLearnPollTimer) stopRxLearnPolling(); showPage(page); if (page === "settings") loadConfig(); if (page === "learn") loadLearnStatus(); if (page === "slots") loadSlots(); if (page === "rxslots") loadRxSlots(); if (page === "analyzer") loadAnalyzer(); }));
  elements.refreshStatusButton.addEventListener("click", loadStatus); elements.refreshAnalyzerButton.addEventListener("click", loadAnalyzer); elements.refreshSlotsButton.addEventListener("click", loadSlots); elements.refreshRxSlotsButton.addEventListener("click", loadRxSlots);
  elements.rxLearnRssiThreshold?.addEventListener("input", queueRxLearnRssiSave);
  elements.rxLearnRssiThreshold?.addEventListener("change", queueRxLearnRssiSave);
  [elements.analyzerRssiThreshold, elements.analyzerMinPulses, elements.analyzerMinDuration, elements.analyzerSimilarity, elements.analyzerOccurrences, elements.analyzerAlternation].forEach(control => {
    control?.addEventListener("input", queueAnalyzerSettingsSave);
    control?.addEventListener("change", queueAnalyzerSettingsSave);
  });
  elements.analyzerShowRejected?.addEventListener("change", queueAnalyzerSettingsSave);
  elements.analyzerFreezeCandidate?.addEventListener("change", queueAnalyzerSettingsSave);
  elements.analyzerDeveloperMode?.addEventListener("change", toggleAnalyzerDeveloperMode);
  elements.mqttEnabled.addEventListener("change", updateMqttFieldState); elements.settingsForm.addEventListener("submit", saveConfig);
  elements.learnStartButton.addEventListener("click", () => learnAction("/api/radio/learn/start")); elements.learnAcceptButton.addEventListener("click", () => learnAction("/api/radio/learn/accept")); elements.learnSaveButton.addEventListener("click", saveLearnToSlot);
  elements.slotSaveSelect.addEventListener("change", updateSlotSaveSelection); elements.slotSaveCancel.addEventListener("click", closeSlotSaveModal); elements.slotSaveConfirm.addEventListener("click", confirmSlotSave); elements.slotSaveModal.addEventListener("click", event => { if (event.target === elements.slotSaveModal) closeSlotSaveModal(); }); elements.learnTestSendButton.addEventListener("click", () => learnAction("/api/radio/learn/test-send")); elements.learnDiscardButton.addEventListener("click", () => learnAction("/api/radio/learn/discard"));
  elements.otaUploadButton.addEventListener("click", installFirmware); elements.backupRestoreButton.addEventListener("click", restoreBackup);
  updateMqttFieldState(); loadStatus(); loadRawFrame(); loadLearnStatus();
  window.setInterval(() => {
    if (!elements.analyzerDeveloperMode?.checked) loadRadioStatus();
  }, 2000);
  window.setInterval(() => {
    if (document.getElementById("page-analyzer").classList.contains("active") &&
        elements.analyzerDeveloperMode?.checked) {
      loadAnalyzerFast();
    }
  }, 100);
  window.setInterval(() => {
    // Analyzer already carries its own RAW preview. Avoid a duplicate RAW API
    // request while the Analyzer page is visible.
    if (!document.getElementById("page-analyzer").classList.contains("active")) {
      loadRawFrame();
    }
  }, 500);
  elements.copyAnalyzerButton?.addEventListener("click", async () => { try { await navigator.clipboard.writeText(elements.analyzerBitstream.textContent); elements.copyAnalyzerButton.textContent = "Copied"; window.setTimeout(() => { elements.copyAnalyzerButton.textContent = "Copy report"; }, 1200); } catch { elements.copyAnalyzerButton.textContent = "Copy failed"; } });
window.setInterval(() => {
  if (!elements.analyzerDeveloperMode?.checked && document.getElementById("page-learn").classList.contains("active")) loadLearnStatus();
}, 1000);
});


// Live header diagnostics: independent from the active page so Core 0/Core 1
// load remains visible while testing RF Learn, Slots, Analyzer, Settings, etc.
window.setInterval(async () => {
  try {
    const d = await requestJson("/api/status");
    setGauge(elements.core0Gauge, elements.core0Load, d.core0_load_percent);
    setGauge(elements.core1Gauge, elements.core1Load, d.core1_load_percent);
    setGauge(elements.psramGauge, elements.psramLoad, d.psram_used_percent);
    setGauge(elements.heapGauge, elements.heapLoad, d.heap_used_percent);
    renderMemoryDiagnostics(d);
  } catch (_) {
    // Keep the last values during a short Wi-Fi/API interruption.
  }
}, 1000);
