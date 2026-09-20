const $ = (id) => document.getElementById(id);
const elements = {
  dashboardRadio1: $("dashboardRadio1"), dashboardRadio2: $("dashboardRadio2"),
  aboutFirmware: $("aboutFirmware"), eventQueueDepth: $("eventQueueDepth"), eventProcessed: $("eventProcessed"), eventDropped: $("eventDropped"),
  connectionBadge: $("connectionBadge"), status: $("status"), firmware: $("firmware"), ip: $("ip"), wifiMode: $("wifiMode"),
  radioChip: $("radioChip"), radioMode: $("radioMode"), radioFrequency: $("radioFrequency"), radioRssi: $("radioRssi"), mqttStatus: $("mqttStatus"), freeHeap: $("freeHeap"), heapFragmentation: $("heapFragmentation"), uptime: $("uptime"),
  rawPulseCount: $("rawPulseCount"), rawDuration: $("rawDuration"), rawRssi: $("rawRssi"), rawSequence: $("rawSequence"), rawAge: $("rawAge"), rawPreview: $("rawPreview"),
  refreshStatusButton: $("refreshStatusButton"), settingsForm: $("settingsForm"), hostname: $("hostname"), replayCount: $("replayCount"), wifiSsid: $("wifiSsid"), wifiPassword: $("wifiPassword"), wifiPasswordState: $("wifiPasswordState"), mqttEnabled: $("mqttEnabled"), mqttFields: $("mqttFields"), mqttHost: $("mqttHost"), mqttPort: $("mqttPort"), mqttUser: $("mqttUser"), mqttPassword: $("mqttPassword"), homeAssistantDiscovery: $("homeAssistantDiscovery"), passwordState: $("passwordState"), saveMessage: $("saveMessage"), saveButton: $("saveButton"),
  analyzerViewBoth: $("analyzerViewBoth"), analyzerViewRadio1: $("analyzerViewRadio1"), analyzerViewRadio2: $("analyzerViewRadio2"), analyzerBothOverview: $("analyzerBothOverview"),
  analyzerSummary1Badge: $("analyzerSummary1Badge"), analyzerSummary1Frequency: $("analyzerSummary1Frequency"), analyzerSummary1Candidate: $("analyzerSummary1Candidate"), analyzerSummary1Frame: $("analyzerSummary1Frame"), analyzerSummary1Rssi: $("analyzerSummary1Rssi"),
  analyzerSummary2Badge: $("analyzerSummary2Badge"), analyzerSummary2Frequency: $("analyzerSummary2Frequency"), analyzerSummary2Candidate: $("analyzerSummary2Candidate"), analyzerSummary2Frame: $("analyzerSummary2Frame"), analyzerSummary2Rssi: $("analyzerSummary2Rssi"),
  analyzerSequence: $("analyzerSequence"), analyzerBadge: $("analyzerBadge"), analyzerModeMessage: $("analyzerModeMessage"), analyzerLatencyState: $("analyzerLatencyState"), analyzerServerAge: $("analyzerServerAge"), analyzerCandidateLatencyAge: $("analyzerCandidateLatencyAge"), analyzerProcessingTime: $("analyzerProcessingTime"), analyzerApiBuildTime: $("analyzerApiBuildTime"), analyzerBrowserRequestTime: $("analyzerBrowserRequestTime"), analyzerFrequency: $("analyzerFrequency"), analyzerRssi: $("analyzerRssi"), analyzerProtocol: $("analyzerProtocol"), analyzerEncoding: $("analyzerEncoding"), analyzerBits: $("analyzerBits"), analyzerBasePulse: $("analyzerBasePulse"), analyzerFrameCount: $("analyzerFrameCount"), analyzerQuality: $("analyzerQuality"), analyzerPulseCount: $("analyzerPulseCount"), analyzerDuration: $("analyzerDuration"), analyzerCode: $("analyzerCode"), analyzerClasses: $("analyzerClasses"), analyzerCandidates: $("analyzerCandidates"), analyzerAccepted: $("analyzerAccepted"), analyzerRejected: $("analyzerRejected"), analyzerDecoded: $("analyzerDecoded"), analyzerUnknown: $("analyzerUnknown"), analyzerDetailTitle: $("analyzerDetailTitle"), analyzerAge: $("analyzerAge"), analyzerBitstream: $("analyzerBitstream"), refreshAnalyzerButton: $("refreshAnalyzerButton"), copyAnalyzerButton: $("copyAnalyzerButton"), analyzerRssiThreshold: $("analyzerRssiThreshold"), analyzerRssiThresholdValue: $("analyzerRssiThresholdValue"), analyzerRssiSaveState: $("analyzerRssiSaveState"), analyzerCurrentRssi: $("analyzerCurrentRssi"), analyzerPeakRssi: $("analyzerPeakRssi"), analyzerWeakRssi: $("analyzerWeakRssi"), analyzerMinPulses: $("analyzerMinPulses"), analyzerMinPulsesValue: $("analyzerMinPulsesValue"), analyzerMinDuration: $("analyzerMinDuration"), analyzerMinDurationValue: $("analyzerMinDurationValue"), analyzerSimilarity: $("analyzerSimilarity"), analyzerSimilarityValue: $("analyzerSimilarityValue"), analyzerOccurrences: $("analyzerOccurrences"), analyzerOccurrencesValue: $("analyzerOccurrencesValue"), analyzerShowRejected: $("analyzerShowRejected"), analyzerFreezeCandidate: $("analyzerFreezeCandidate"), analyzerAlternation: $("analyzerAlternation"), analyzerAlternationValue: $("analyzerAlternationValue"), analyzerDeveloperMode: $("analyzerDeveloperMode"), developerModeState: $("developerModeState"), developerExclusiveWarning: $("developerExclusiveWarning"), candidateDeveloperMetrics: $("candidateDeveloperMetrics"), candidateAlternation: $("candidateAlternation"), candidateSamePairs: $("candidateSamePairs"), candidateLongestRun: $("candidateLongestRun"), candidateNormalizedCount: $("candidateNormalizedCount"), candidateNormalizedRaw: $("candidateNormalizedRaw"), candidateSequence: $("candidateSequence"), candidateAge: $("candidateAge"), candidateRssi: $("candidateRssi"), candidatePulseCount: $("candidatePulseCount"), candidateDuration: $("candidateDuration"), candidateRejectReason: $("candidateRejectReason"), candidateRaw: $("candidateRaw"),
  learnState: $("learnState"), learnBadge: $("learnBadge"), learnMessage: $("learnMessage"), learnStartButton: $("learnStartButton"), learnAcceptButton: $("learnAcceptButton"), learnSaveButton: $("learnSaveButton"), learnTestSendButton: $("learnTestSendButton"), learnDiscardButton: $("learnDiscardButton"), learnPulseCount: $("learnPulseCount"), learnDuration: $("learnDuration"), learnRssi: $("learnRssi"), learnNoiseFloor: $("learnNoiseFloor"), learnRejected: $("learnRejected"), learnRejectReason: $("learnRejectReason"), learnPreviewTitle: $("learnPreviewTitle"), learnRawPreview: $("learnRawPreview"),
  refreshSlotsButton: $("refreshSlotsButton"), slotUsage: $("slotUsage"), slotMessage: $("slotMessage"), slotGrid: $("slotGrid"),
  refreshRxSlotsButton: $("refreshRxSlotsButton"), rxSlotUsage: $("rxSlotUsage"), rxSlotMessage: $("rxSlotMessage"), rxSlotGrid: $("rxSlotGrid"),
  rxV2TxResult: $("rxV2TxResult"), rxV2TxProtocol: $("rxV2TxProtocol"), rxV2TxRadio: $("rxV2TxRadio"), rxV2TxRepeats: $("rxV2TxRepeats"), rxV2TxAge: $("rxV2TxAge"),
  rxLearnRssiThreshold: $("rxLearnRssiThreshold"), rxLearnRssiValue: $("rxLearnRssiValue"), rxLearnRssiSaveState: $("rxLearnRssiSaveState"),
  slotSaveModal: $("slotSaveModal"), slotSaveSelect: $("slotSaveSelect"), slotSaveName: $("slotSaveName"), slotSaveWarning: $("slotSaveWarning"), slotSaveCancel: $("slotSaveCancel"), slotSaveConfirm: $("slotSaveConfirm"),
  otaFile: $("otaFile"), otaUploadButton: $("otaUploadButton"), otaProgress: $("otaProgress"), otaMessage: $("otaMessage"),
  backupFile: $("backupFile"), backupRestoreButton: $("backupRestoreButton"), backupProgress: $("backupProgress"), backupMessage: $("backupMessage"), uiVersion: $("uiVersion"),
  memoryDiagState: $("memoryDiagState"), memoryFlashTotal: $("memoryFlashTotal"),
  memoryPsramTotal: $("memoryPsramTotal"), memoryPsramFree: $("memoryPsramFree"),
  memoryHeapTotal: $("memoryHeapTotal"), memoryHeapFree: $("memoryHeapFree"), memoryOpenrfPsram: $("memoryOpenrfPsram"), memoryAnalyzerPsram: $("memoryAnalyzerPsram"),
  rfHardwareState: $("rfHardwareState"), rf1HardwareState: $("rf1HardwareState"), rf2HardwareState: $("rf2HardwareState"), loraHardwareState: $("loraHardwareState"),
  rf1Enabled: $("rf1Enabled"), rf2Enabled: $("rf2Enabled"), loraEnabled: $("loraEnabled"), saveRfEnableButton: $("saveRfEnableButton"), rfEnableMessage: $("rfEnableMessage"),
  rf1EngineState: $("rf1EngineState"), rf2EngineState: $("rf2EngineState"), loraEngineState: $("loraEngineState"),
  rf1FrequencyState: $("rf1FrequencyState"), rf2FrequencyState: $("rf2FrequencyState"),
  rf1DiagEdges: $("rf1DiagEdges"), rf1DiagCandidates: $("rf1DiagCandidates"), rf1DiagAccepted: $("rf1DiagAccepted"), rf1DiagRejected: $("rf1DiagRejected"), rf1DiagPulses: $("rf1DiagPulses"),
  rf2DiagEdges: $("rf2DiagEdges"), rf2DiagCandidates: $("rf2DiagCandidates"), rf2DiagAccepted: $("rf2DiagAccepted"), rf2DiagRejected: $("rf2DiagRejected"), rf2DiagPulses: $("rf2DiagPulses"),
  rf1DiagFinalizeReason: $("rf1DiagFinalizeReason"), rf1DiagLastFrame: $("rf1DiagLastFrame"), rf1DiagValidation: $("rf1DiagValidation"), rf1DiagFinalizeCounts: $("rf1DiagFinalizeCounts"), rf1DiagShortResets: $("rf1DiagShortResets"), rf1DiagIgnoredReady: $("rf1DiagIgnoredReady"), rf1DiagGlitches: $("rf1DiagGlitches"), rf1DiagPending: $("rf1DiagPending"),
  rf2DiagFinalizeReason: $("rf2DiagFinalizeReason"), rf2DiagLastFrame: $("rf2DiagLastFrame"), rf2DiagValidation: $("rf2DiagValidation"), rf2DiagFinalizeCounts: $("rf2DiagFinalizeCounts"), rf2DiagShortResets: $("rf2DiagShortResets"), rf2DiagIgnoredReady: $("rf2DiagIgnoredReady"), rf2DiagGlitches: $("rf2DiagGlitches"), rf2DiagPending: $("rf2DiagPending"),
  protocolDiagState: $("protocolDiagState"), protocolDiagRadio: $("protocolDiagRadio"), protocolDiagAge: $("protocolDiagAge"), protocolDiagRaw: $("protocolDiagRaw"), protocolDiagFrequency: $("protocolDiagFrequency"), protocolDiagRssi: $("protocolDiagRssi"), protocolDiagV2: $("protocolDiagV2"), protocolDiagV2Result: $("protocolDiagV2Result"), protocolLastKnownProtocol: $("protocolLastKnownProtocol"), protocolLastKnownCode: $("protocolLastKnownCode"), protocolLastKnownSource: $("protocolLastKnownSource"), protocolLastKnownAge: $("protocolLastKnownAge"), protocolDiagRegistry: $("protocolDiagRegistry"), protocolDiagCounts: $("protocolDiagCounts"), protocolEngineDecision: $("protocolEngineDecision"), protocolEngineMatched: $("protocolEngineMatched"), protocolEngineSelected: $("protocolEngineSelected"), protocolEngineCandidates: $("protocolEngineCandidates"), protocolEngineUnknownCount: $("protocolEngineUnknownCount"), protocolEngineKnownCount: $("protocolEngineKnownCount"), protocolEngineAmbiguousCount: $("protocolEngineAmbiguousCount"), protocolNormalizedAvailable: $("protocolNormalizedAvailable"), protocolNormalizedProtocol: $("protocolNormalizedProtocol"), protocolNormalizedCode: $("protocolNormalizedCode"), protocolNormalizedSymbols: $("protocolNormalizedSymbols"), protocolNormalizedRepeats: $("protocolNormalizedRepeats"), protocolNormalizedRadio: $("protocolNormalizedRadio"), protocolNormalizedFrequency: $("protocolNormalizedFrequency"), protocolNormalizedRssi: $("protocolNormalizedRssi"), protocolNormalizedRaw: $("protocolNormalizedRaw"), protocolNormalizedCount: $("protocolNormalizedCount"), protocolDedupState: $("protocolDedupState"), protocolDedupWindow: $("protocolDedupWindow"), protocolDedupDelta: $("protocolDedupDelta"), protocolDedupBurst: $("protocolDedupBurst"), protocolDedupInputCount: $("protocolDedupInputCount"), protocolDedupLogicalCount: $("protocolDedupLogicalCount"), protocolDedupCollapsedCount: $("protocolDedupCollapsedCount"), protocolNvkpResult: $("protocolNvkpResult"), protocolNvkpReject: $("protocolNvkpReject"), protocolNvkpMarkers: $("protocolNvkpMarkers"), protocolNvkpSync: $("protocolNvkpSync"), protocolNvkpLeader: $("protocolNvkpLeader"), protocolNvkpEnvelope: $("protocolNvkpEnvelope"), protocolNvkpAlternating: $("protocolNvkpAlternating"), protocolNvkpCode: $("protocolNvkpCode"), protocolNvkpRepeats: $("protocolNvkpRepeats"), protocolNvkpRejectCounts: $("protocolNvkpRejectCounts"), protocolHt12eResult: $("protocolHt12eResult"), protocolHt12eReject: $("protocolHt12eReject"), protocolHt12eWords: $("protocolHt12eWords"), protocolHt12eT: $("protocolHt12eT"), protocolHt12eTiming: $("protocolHt12eTiming"), protocolHt12eCode: $("protocolHt12eCode"), protocolHt12eAddressData: $("protocolHt12eAddressData"), protocolHt12eRepeats: $("protocolHt12eRepeats"), protocolHt12eRejectCounts: $("protocolHt12eRejectCounts"), protocolV2AuthoritativeState: $("protocolV2AuthoritativeState"), protocolV2Route: $("protocolV2Route"), protocolV2RouteCounts: $("protocolV2RouteCounts"), rawMatcherState: $("rawMatcherState"), rawMatcherMatch: $("rawMatcherMatch"), rawMatcherScores: $("rawMatcherScores"), rawMatcherPulses: $("rawMatcherPulses"), rawMatcherRepeat: $("rawMatcherRepeat"), rawMatcherCounts: $("rawMatcherCounts"),
  
  protocolEvRejectReason: $("protocolEvRejectReason"), protocolEvDecodedCode: $("protocolEvDecodedCode"), protocolEvFrames: $("protocolEvFrames"), protocolEvFrameIndexes: $("protocolEvFrameIndexes"), protocolEvAddressCheck: $("protocolEvAddressCheck"), protocolEvCommandCheck: $("protocolEvCommandCheck"), protocolEvBaseT: $("protocolEvBaseT"), protocolEvShort: $("protocolEvShort"), protocolEvLong: $("protocolEvLong"), protocolEvRatio: $("protocolEvRatio"), protocolEvSync: $("protocolEvSync"), protocolEvRepeats: $("protocolEvRepeats"), protocolEvRepeatDeviation: $("protocolEvRepeatDeviation"), protocolEvRejectCounts: $("protocolEvRejectCounts"),
  protocolPtResult: $("protocolPtResult"), protocolPtRejectReason: $("protocolPtRejectReason"), protocolPtDecodedCode: $("protocolPtDecodedCode"), protocolPtFrames: $("protocolPtFrames"), protocolPtFrameIndexes: $("protocolPtFrameIndexes"), protocolPtTrits: $("protocolPtTrits"), protocolPtBaseT: $("protocolPtBaseT"), protocolPtShort: $("protocolPtShort"), protocolPtLong: $("protocolPtLong"), protocolPtRatio: $("protocolPtRatio"), protocolPtSync: $("protocolPtSync"), protocolPtRepeats: $("protocolPtRepeats"), protocolPtRepeatDeviation: $("protocolPtRepeatDeviation"), protocolPtRejectCounts: $("protocolPtRejectCounts"),
  frequencyScan433State: $("frequencyScan433State"), frequencyScan433Start: $("frequencyScan433Start"), frequencyScan433End: $("frequencyScan433End"), frequencyScan433Step: $("frequencyScan433Step"), frequencyScan433Dwell: $("frequencyScan433Dwell"), frequencyScan433Passes: $("frequencyScan433Passes"), frequencyScan433Button: $("frequencyScan433Button"), frequencyScan433BestFrequency: $("frequencyScan433BestFrequency"), frequencyScan433EstimatedCarrier: $("frequencyScan433EstimatedCarrier"), frequencyScan433BestRssi: $("frequencyScan433BestRssi"), frequencyScan433NoiseFloor: $("frequencyScan433NoiseFloor"), frequencyScan433Delta: $("frequencyScan433Delta"), frequencyScan433Detection: $("frequencyScan433Detection"), frequencyScan433SampleCount: $("frequencyScan433SampleCount"), frequencyScan433Bars: $("frequencyScan433Bars"),
  frequencyScan433TuneButton: $("frequencyScan433TuneButton"), frequencyScan433RestoreButton: $("frequencyScan433RestoreButton"), frequencyScan433TuneState: $("frequencyScan433TuneState"),
  frequencyScan868State: $("frequencyScan868State"), frequencyScan868Start: $("frequencyScan868Start"), frequencyScan868End: $("frequencyScan868End"), frequencyScan868Step: $("frequencyScan868Step"), frequencyScan868Dwell: $("frequencyScan868Dwell"), frequencyScan868Passes: $("frequencyScan868Passes"), frequencyScan868Button: $("frequencyScan868Button"), frequencyScan868BestFrequency: $("frequencyScan868BestFrequency"), frequencyScan868EstimatedCarrier: $("frequencyScan868EstimatedCarrier"), frequencyScan868BestRssi: $("frequencyScan868BestRssi"), frequencyScan868NoiseFloor: $("frequencyScan868NoiseFloor"), frequencyScan868Delta: $("frequencyScan868Delta"), frequencyScan868Detection: $("frequencyScan868Detection"), frequencyScan868SampleCount: $("frequencyScan868SampleCount"), frequencyScan868Bars: $("frequencyScan868Bars"),
  frequencyScan868TuneButton: $("frequencyScan868TuneButton"), frequencyScan868RestoreButton: $("frequencyScan868RestoreButton"), frequencyScan868TuneState: $("frequencyScan868TuneState"),
  rf1TuneHeader: $("rf1TuneHeader"), rf2TuneHeader: $("rf2TuneHeader"),
  core0Gauge: $("core0Gauge"), core0Load: $("core0Load"), core1Gauge: $("core1Gauge"), core1Load: $("core1Load"),
  psramGauge: $("psramGauge"), psramLoad: $("psramLoad"), heapGauge: $("heapGauge"), heapLoad: $("heapLoad")
};



function formatMemoryBytes(value) {
  const bytes = Number(value || 0);
  if (!bytes) return "0 MB";
  return `${(bytes / (1024 * 1024)).toFixed(2)} MB`;
}

function setHardwareBadge(element, online) {
  if (!element) return;
  element.textContent = online ? "ONLINE" : "OFFLINE";
  element.className = `badge ${online ? "badge-online" : "badge-error"}`;
}

function formatTuneCountdown(ms) {
  const totalSeconds = Math.max(0, Math.ceil(Number(ms || 0) / 1000));
  const minutes = Math.floor(totalSeconds / 60);
  const seconds = totalSeconds % 60;
  return `${String(minutes).padStart(2,"0")}:${String(seconds).padStart(2,"0")}`;
}

function renderTuneHeaderStatus(d) {
  const apply = (el, label, tuned, remainingMs) => {
    if (!el) return;
    const remaining = Number(remainingMs || 0);
    const active = !!tuned && remaining > 0;
    el.hidden = !active;
    if (active) el.textContent = `${label} TUNED · ${formatTuneCountdown(remaining)}`;
  };
  apply(elements.rf1TuneHeader, "433", d.rf1_frequency_tuned, d.rf1_tune_remaining_ms);
  apply(elements.rf2TuneHeader, "868", d.rf2_frequency_tuned, d.rf2_tune_remaining_ms);
}

let savedLoraEnabled = null;
function renderRfHardwareStatus(d) {
  if (typeof d.lora_enabled === "boolean") { savedLoraEnabled = d.lora_enabled; elements.saveRfEnableButton.disabled = false; }
  renderTuneHeaderStatus(d);
  setHardwareBadge(elements.rf1HardwareState, !!d.rf1_online);
  setHardwareBadge(elements.rf2HardwareState, !!d.rf2_online);
  setHardwareBadge(elements.loraHardwareState, !!d.lora_online);
  elements.loraHardwareState.textContent = d.lora_online ? "Detected – integration pending" : "Not detected";

  if (elements.rf1Enabled) elements.rf1Enabled.checked = !!d.rf1_enabled;
  if (elements.rf2Enabled) elements.rf2Enabled.checked = !!d.rf2_enabled;
  if (elements.loraEnabled) elements.loraEnabled.checked = !!d.lora_enabled;

  if (elements.rf1EngineState) {
    elements.rf1EngineState.textContent = `Engine: ${d.rf1_active ? "ACTIVE" : "INACTIVE"}`;
  }
  if (elements.rf2EngineState) {
    elements.rf2EngineState.textContent = `Engine: ${d.rf2_active ? "ACTIVE" : "INACTIVE"}`;
  }
  if (elements.loraEngineState) {
    elements.loraEngineState.textContent =
      "RF engine: integration pending";
  }

  const renderFrequencyState = (element, defaultValue, operatingValue, tuned) => {
    if (!element) return;
    const def = Number(defaultValue);
    const op = Number(operatingValue);
    const defText = Number.isFinite(def) && def > 0 ? def.toFixed(3) : "—";
    const opText = Number.isFinite(op) && op > 0 ? op.toFixed(4) : "—";
    element.textContent = `Default: ${defText} MHz · Operating: ${opText} MHz${tuned ? " · TUNED" : ""}`;
  };
  renderFrequencyState(elements.rf1FrequencyState, d.rf1_default_frequency_mhz, d.rf1_operating_frequency_mhz, !!d.rf1_frequency_tuned);
  renderFrequencyState(elements.rf2FrequencyState, d.rf2_default_frequency_mhz, d.rf2_operating_frequency_mhz, !!d.rf2_frequency_tuned);

  if (elements.rfHardwareState) {
    const onlineCount = [d.rf1_online, d.rf2_online, d.lora_online].filter(Boolean).length;
    elements.rfHardwareState.textContent = `${onlineCount}/3 detected`;
    elements.rfHardwareState.className =
      "form-message";
  }
}

const frequencyScanEstimatedCarrier = {1: null, 2: null};

function renderFrequencyScan(samples, barsElement) {
  if (!barsElement) return;

  if (!Array.isArray(samples) || !samples.length) {
    barsElement.innerHTML = '<span class="platform-note">No samples.</span>';
    return;
  }

  const values = samples.map(point => Number(point.rssi_dbm));
  const floor = Math.min(...values, -115);
  const ceiling = Math.max(...values, -40);
  const span = Math.max(1, ceiling - floor);

  barsElement.innerHTML = samples.map(point => {
    const rssi = Number(point.rssi_dbm);
    const frequency = Number(point.frequency_mhz);
    const height = Math.max(4, Math.min(100, ((rssi - floor) / span) * 100));

    return `<div class="frequency-scan-bar-wrap" title="${frequency.toFixed(3)} MHz · ${rssi.toFixed(1)} dBm">
      <div class="frequency-scan-bar" style="height:${height.toFixed(1)}%"></div>
      <span>${frequency.toFixed(3)}</span>
    </div>`;
  }).join("");
}

async function runFrequencyScan(radioId) {
  const is433 = radioId === 1;

  const button = is433 ? elements.frequencyScan433Button : elements.frequencyScan868Button;
  const state = is433 ? elements.frequencyScan433State : elements.frequencyScan868State;
  const startElement = is433 ? elements.frequencyScan433Start : elements.frequencyScan868Start;
  const endElement = is433 ? elements.frequencyScan433End : elements.frequencyScan868End;
  const stepElement = is433 ? elements.frequencyScan433Step : elements.frequencyScan868Step;
  const dwellElement = is433 ? elements.frequencyScan433Dwell : elements.frequencyScan868Dwell;
  const passesElement = is433 ? elements.frequencyScan433Passes : elements.frequencyScan868Passes;
  const bestFrequency = is433 ? elements.frequencyScan433BestFrequency : elements.frequencyScan868BestFrequency;
  const estimatedCarrier = is433 ? elements.frequencyScan433EstimatedCarrier : elements.frequencyScan868EstimatedCarrier;
  const bestRssi = is433 ? elements.frequencyScan433BestRssi : elements.frequencyScan868BestRssi;
  const noiseFloor = is433 ? elements.frequencyScan433NoiseFloor : elements.frequencyScan868NoiseFloor;
  const signalDelta = is433 ? elements.frequencyScan433Delta : elements.frequencyScan868Delta;
  const detection = is433 ? elements.frequencyScan433Detection : elements.frequencyScan868Detection;
  const sampleCount = is433 ? elements.frequencyScan433SampleCount : elements.frequencyScan868SampleCount;
  const bars = is433 ? elements.frequencyScan433Bars : elements.frequencyScan868Bars;
  const tuneButton = is433 ? elements.frequencyScan433TuneButton : elements.frequencyScan868TuneButton;
  const tuneState = is433 ? elements.frequencyScan433TuneState : elements.frequencyScan868TuneState;

  if (!button) return;

  const start = Number(startElement?.value || (is433 ? 433.6 : 867.8));
  const end = Number(endElement?.value || (is433 ? 434.2 : 868.9));
  const step = Number(stepElement?.value || 0.025);
  const dwell = Number(dwellElement?.value || 35);
  const passes = Number(passesElement?.value || 4);

  button.disabled = true;

  if (state) {
    state.textContent = "Scanning — press the remote now";
    state.className = "form-message";
  }

  try {
    const query = new URLSearchParams({
      radio: String(radioId),
      start: start.toFixed(3),
      end: end.toFixed(3),
      step: step.toFixed(3),
      dwell: String(dwell),
      passes: String(passes)
    });

    const result = await requestJson(`/api/radio/frequency-scan?${query.toString()}`);
    const samples = Array.isArray(result.samples) ? result.samples : [];

    bestFrequency.textContent =
      `${Number(result.strongest_frequency_mhz).toFixed(3)} MHz`;

    const estimated = Number(result.estimated_carrier_mhz);
    estimatedCarrier.textContent =
      Number.isFinite(estimated) ? `${estimated.toFixed(4)} MHz` : "—";

    frequencyScanEstimatedCarrier[radioId] =
      result.signal_detected && Number.isFinite(estimated) ? estimated : null;

    if (tuneButton) {
      tuneButton.disabled = !frequencyScanEstimatedCarrier[radioId];
    }
    if (tuneState) {
      tuneState.textContent = "";
      tuneState.className = "form-message";
    }

    bestRssi.textContent =
      `${Number(result.strongest_rssi_dbm).toFixed(1)} dBm`;
    noiseFloor.textContent =
      `${Number(result.noise_floor_dbm).toFixed(1)} dBm`;
    signalDelta.textContent =
      `+${Number(result.signal_above_noise_db).toFixed(1)} dB`;
    detection.textContent =
      result.signal_detected ? String(result.signal_quality || "DETECTED") : "NO SIGNAL";
    detection.className =
      `scan-detection ${result.signal_detected ? "scan-detection-ok" : "scan-detection-none"}`;
    sampleCount.textContent = String(samples.length);

    renderFrequencyScan(samples, bars);

    if (state) {
      state.textContent = "Scan complete";
      state.className = "form-message success";
    }
  } catch (error) {
    if (state) {
      state.textContent = error.message;
      state.className = "form-message error";
    }
  } finally {
    button.disabled = false;
  }
}


async function tuneRadioToDetectedCarrier(radioId) {
  const frequency = frequencyScanEstimatedCarrier[radioId];
  const is433 = radioId === 1;
  const button = is433 ? elements.frequencyScan433TuneButton : elements.frequencyScan868TuneButton;
  const state = is433 ? elements.frequencyScan433TuneState : elements.frequencyScan868TuneState;

  if (!Number.isFinite(frequency) || !button) return;

  button.disabled = true;
  if (state) {
    state.textContent = `Tuning to ${frequency.toFixed(4)} MHz...`;
    state.className = "form-message";
  }

  try {
    const result = await postJson("/api/radio/frequency-tune", {
      radio: radioId,
      frequency_mhz: frequency
    });

    if (state) {
      state.textContent = `Operating: ${Number(result.frequency_mhz).toFixed(4)} MHz${result.frequency_tuned ? " · TUNED · 15:00" : ""}`;
      state.className = "form-message success";
    }

    // Step 27.1 UI sync: after Tune, refresh the same two sources used by
    // Restore Default so System > Installed radio modules and the header
    // TUNED badge/countdown update immediately without a browser refresh.
    await loadRadioStatus(true);
    const status = await requestJson("/api/status");
    renderRfHardwareStatus(status);
  } catch (error) {
    if (state) {
      state.textContent = error.message;
      state.className = "form-message error";
    }
  } finally {
    button.disabled = false;
  }
}

async function saveRfEnableSettings() {
  if (!elements.saveRfEnableButton || savedLoraEnabled === null) return;

  elements.saveRfEnableButton.disabled = true;
  if (elements.rfEnableMessage) {
    elements.rfEnableMessage.textContent = "Saving...";
    elements.rfEnableMessage.className = "form-message";
  }

  try {
    const result = await postJson("/api/system/radios", {
      radio1_enabled: !!elements.rf1Enabled?.checked,
      radio2_enabled: !!elements.rf2Enabled?.checked,
      lora_enabled: savedLoraEnabled
    });

    if (elements.rfEnableMessage) {
      elements.rfEnableMessage.textContent = result.message || "Radio settings saved.";
      elements.rfEnableMessage.className = "form-message success";
    }
  } catch (error) {
    if (elements.rfEnableMessage) {
      elements.rfEnableMessage.textContent = error.message;
      elements.rfEnableMessage.className = "form-message error";
    }
    elements.saveRfEnableButton.disabled = false;
  }
}

// Dashboard uses the hardware snapshot from /api/status, not /api/radio.
function renderDashboardRadios(d) {
  for (const id of [1, 2]) {
    const enabled = d[`rf${id}_enabled`];
    const online = d[`rf${id}_online`];
    const active = d[`rf${id}_active`];
    const operating = Number(d[`rf${id}_operating_frequency_mhz`]);
    const fallback = Number(d[`rf${id}_default_frequency_mhz`]);
    const frequency = Number.isFinite(operating) && operating > 0 ? operating : fallback;
    const status = enabled === false ? "Disabled"
      : online === false ? "Not detected"
      : active === true ? "Active"
      : active === false ? "Inactive" : "—";
    elements[`dashboardRadio${id}`].textContent = `${status} · ${Number.isFinite(frequency) && frequency > 0 ? frequency.toFixed(3) + " MHz" : "—"}`;
  }
}

function renderFirmwareVersion(d) {
  const version = d.version ? `v${String(d.version).replace(/^v/, "")}` : "—";
  [elements.firmware, elements.uiVersion, elements.aboutFirmware].forEach(el => { if (el) el.textContent = version; });
}
function renderMemoryDiagnostics(d) {
  elements.eventQueueDepth.textContent = d.rf_event_queue_depth ?? "—";
  elements.eventProcessed.textContent = d.rf_event_processed ?? "—";
  elements.eventDropped.textContent = d.rf_event_dropped ?? "—";
  elements.freeHeap.textContent = d.free_heap == null ? "—" : `${Number(d.free_heap).toLocaleString()} B`;
  elements.heapFragmentation.textContent = d.heap_fragmentation_percent == null ? "—" : `${d.heap_fragmentation_percent}%`;

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
const OPENRF_PAGES = new Set([
  "dashboard", "learn", "slots", "rxslots", "analyzer", "settings", "system", "diagnostics", "about"
]);

function pageFromHash() {
  const requested = String(window.location.hash || "")
    .replace(/^#/, "")
    .trim()
    .toLowerCase();
  return OPENRF_PAGES.has(requested) ? requested : "dashboard";
}

function showPage(name, updateHash = true) {
  const page = OPENRF_PAGES.has(name) ? name : "dashboard";

  document.querySelectorAll(".page").forEach(element => {
    element.classList.toggle("active", element.id === `page-${page}`);
  });
  document.querySelectorAll(".tab-button").forEach(button => {
    button.classList.toggle("active", button.dataset.page === page);
  });

  // Keep the active OpenRF page through a browser refresh/reconnect without
  // forcing a full navigation. Example: /#system or /#analyzer.
  if (updateHash && window.location.hash !== `#${page}`) {
    window.history.replaceState(null, "", `#${page}`);
  }

  return page;
}

function activatePage(name, updateHash = true) {
  const page = showPage(name, updateHash);

  if (page !== "rxslots" && rxLearnPollTimer) stopRxLearnPolling();
  if (page === "settings") loadConfig();
  if (page === "learn") loadLearnStatus();
  if (page === "slots") loadSlots();
  if (page === "rxslots") loadRxSlots();
  if (page === "analyzer") {
    if (analyzerViewMode === "both") loadAnalyzerBothSummary();
    else loadAnalyzer();
  }
  if (page === "system") {
    // System contains hardware/maintenance information. Refresh the hardware
    // snapshot when the page is opened.
    loadStatus();
  }
  if (page === "diagnostics") {
    loadProtocolTx();
    // Diagnostics owns the live RF capture + Protocol Engine instrumentation.
    loadRadioStatus(true);
  }

  return page;
}
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

function formatDiagnosticDurationMs(ms) {
  const value = Math.max(0, Number(ms || 0));
  if (value < 1000) return `${value.toFixed(0)} ms`;
  if (value < 60000) return `${(value / 1000).toFixed(1)} s`;
  if (value < 3600000) {
    const totalSeconds = Math.floor(value / 1000);
    return `${Math.floor(totalSeconds / 60)}m ${totalSeconds % 60}s`;
  }
  if (value < 86400000) {
    const totalMinutes = Math.floor(value / 60000);
    return `${Math.floor(totalMinutes / 60)}h ${totalMinutes % 60}m`;
  }
  const totalHours = Math.floor(value / 3600000);
  return `${Math.floor(totalHours / 24)}d ${totalHours % 24}h`;
}

function renderCaptureFinalizationDiagnostics(radioId, data) {
  const is1 = radioId === 1;
  const finalizeReason = is1 ? elements.rf1DiagFinalizeReason : elements.rf2DiagFinalizeReason;
  const lastFrame = is1 ? elements.rf1DiagLastFrame : elements.rf2DiagLastFrame;
  const validation = is1 ? elements.rf1DiagValidation : elements.rf2DiagValidation;
  const finalizeCounts = is1 ? elements.rf1DiagFinalizeCounts : elements.rf2DiagFinalizeCounts;
  const shortResets = is1 ? elements.rf1DiagShortResets : elements.rf2DiagShortResets;
  const ignoredReady = is1 ? elements.rf1DiagIgnoredReady : elements.rf2DiagIgnoredReady;
  const glitches = is1 ? elements.rf1DiagGlitches : elements.rf2DiagGlitches;
  const pending = is1 ? elements.rf1DiagPending : elements.rf2DiagPending;

  if (finalizeReason) finalizeReason.textContent = data.last_finalize_reason || "—";
  if (lastFrame) {
    const pulses = Number(data.last_finalized_pulses || 0);
    const durationUs = Number(data.last_finalized_duration_us || 0);
    const ageMs = Number(data.last_finalized_age_ms || 0);
    lastFrame.textContent = pulses
      ? `${pulses} pulses · ${(durationUs / 1000).toFixed(2)} ms · ${formatAge(ageMs)}`
      : "—";
  }
  if (validation) validation.textContent = data.last_validation_result || "—";
  if (finalizeCounts) {
    finalizeCounts.textContent =
      `${Number(data.gap_finalized || 0)} / ${Number(data.timeout_finalized || 0)} / ${Number(data.stale_partial_finalized || 0)} / ${Number(data.buffer_full || 0)}`;
  }
  if (shortResets) shortResets.textContent = Number(data.short_gap_resets || 0);
  if (ignoredReady) ignoredReady.textContent = Number(data.ignored_while_frame_ready || 0);
  if (glitches) glitches.textContent = Number(data.ignored_glitch_edges || 0);
  if (pending) {
    const ageUs = Number(data.pending_age_us || 0);
    const ageText = formatDiagnosticDurationMs(ageUs / 1000);
    pending.textContent =
      data.frame_ready ? `YES · pending ${ageText}` : `NO · idle ${ageText}`;
  }
}

let radioStatusRequestActive = false;


async function restoreRadioDefault(radioId) {
  const is433 = radioId === 1;
  const button = is433 ? elements.frequencyScan433RestoreButton : elements.frequencyScan868RestoreButton;
  const state = is433 ? elements.frequencyScan433TuneState : elements.frequencyScan868TuneState;
  if (button) button.disabled = true;
  try {
    const result = await postJson("/api/radio/frequency-restore", {radio: radioId});
    if (state) {
      state.textContent = `Default restored: ${Number(result.frequency_mhz).toFixed(4)} MHz`;
      state.className = "form-message success";
    }
    // Step 27 UI sync: /api/radio updates diagnostics, while the
    // "Installed radio modules" cards are rendered from /api/status.
    // Refresh both immediately after Restore Default.
    await loadRadioStatus(true);
    const status = await requestJson("/api/status");
    renderRfHardwareStatus(status);
  } catch (error) {
    if (state) {
      state.textContent = error.message;
      state.className = "form-message error";
    }
  } finally {
    if (button) button.disabled = false;
  }
}


function renderProtocolDiagnostics(d) {
  const p = d?.protocol_diagnostics || {};
  const authoritative = p.v2_authoritative_action || {};
  if (elements.protocolV2AuthoritativeState) {
    elements.protocolV2AuthoritativeState.textContent =
      "V2-ONLY ACTIVE · Learned RAW fallback enabled";
    elements.protocolV2AuthoritativeState.className = "form-message success";
  }
  if (elements.protocolV2Route) {
    const lastProtocol = authoritative.last_protocol && authoritative.last_protocol !== "Unknown"
      ? ` · ${authoritative.last_protocol}${authoritative.last_code ? ` ${authoritative.last_code}` : ""}`
      : "";
    elements.protocolV2Route.textContent =
      `${authoritative.route_state || "V2_FIRST_READY"}${lastProtocol}`;
  }
  if (elements.protocolV2RouteCounts) {
    elements.protocolV2RouteCounts.textContent =
      `V2 emit ${Number(authoritative.v2_emit_count || 0)} · duplicates suppressed ${Number(authoritative.duplicate_suppressed_count || 0)} · NVKP confirm waits ${Number(authoritative.nvkp_confirmation_pending_count || 0)} · unknown/no action ${Number(authoritative.unknown_no_action_count || 0)}`;
  }

  const rawMatcher = p.learned_raw_matcher || {};
  if (elements.rawMatcherState) elements.rawMatcherState.textContent = rawMatcher.state || "IDLE";
  if (elements.rawMatcherMatch) {
    const slot = Number(rawMatcher.slot || 0);
    const similarity = Number(rawMatcher.similarity || 0);
    elements.rawMatcherMatch.textContent = slot > 0 ? `RF Slot ${slot} · ${similarity}%` : "—";
  }
  if (elements.rawMatcherScores) {
    elements.rawMatcherScores.textContent = `${Number(rawMatcher.timing_similarity || 0)}% / ${Number(rawMatcher.count_similarity || 0)}% / ${Number(rawMatcher.sign_agreement || 0)}%`;
  }
  if (elements.rawMatcherPulses) {
    elements.rawMatcherPulses.textContent = `${Number(rawMatcher.learned_pattern_pulses || 0)} / ${Number(rawMatcher.incoming_pattern_pulses || 0)} / ${Number(rawMatcher.compared_pulses || 0)}`;
  }
  if (elements.rawMatcherRepeat) {
    elements.rawMatcherRepeat.textContent = `learned ${rawMatcher.learned_repeat_reduced ? "reduced" : "full"} · incoming ${rawMatcher.incoming_repeat_reduced ? "reduced" : "full"}`;
  }
  if (elements.rawMatcherCounts) {
    elements.rawMatcherCounts.textContent = `matches ${Number(rawMatcher.match_count || 0)} · no match ${Number(rawMatcher.no_match_count || 0)} · duplicates ${Number(rawMatcher.duplicate_suppressed_count || 0)}`;
  }

  if (!p.available) {
    if (elements.protocolDiagState) elements.protocolDiagState.textContent = "Waiting for RF";
    return;
  }

  if (elements.protocolDiagState) {
    elements.protocolDiagState.textContent = p.frame_accepted ? "Same-capture snapshot · accepted" : "Same-capture snapshot · rejected";
  }
  if (elements.protocolDiagRadio) elements.protocolDiagRadio.textContent = p.radio_id ? `RF${p.radio_id}` : "—";
  if (elements.protocolDiagAge) elements.protocolDiagAge.textContent = formatAge(Number(p.age_ms || 0));
  if (elements.protocolDiagRaw) elements.protocolDiagRaw.textContent = `${Number(p.pulse_count || 0)} pulses · ${(Number(p.duration_us || 0) / 1000).toFixed(2)} ms`;
  if (elements.protocolDiagFrequency) elements.protocolDiagFrequency.textContent = `${Number(p.frequency_mhz || 0).toFixed(4)} MHz`;
  if (elements.protocolDiagRssi) elements.protocolDiagRssi.textContent = `${Number(p.rssi_dbm ?? -127).toFixed(1)} dBm`;

  if (elements.protocolDiagV2) elements.protocolDiagV2.textContent = p.v2_evaluated ? (p.v2_protocol || "Unknown") : "Not evaluated";
  if (elements.protocolDiagV2Result) {
    elements.protocolDiagV2Result.textContent = p.v2_evaluated ? (p.v2_result || "NO_MATCH") : "—";
  }

  // Step 35.1: this field is intentionally latched by the firmware and only
  // changes when another confidently KNOWN V2 event arrives. Noise/UNKNOWN
  // captures continue to update the live same-capture fields above without
  // erasing the last useful recognition.
  const lastKnown = p.v2_last_known_event || {};
  const lastKnownAvailable = !!lastKnown.available;
  if (elements.protocolLastKnownProtocol) elements.protocolLastKnownProtocol.textContent = lastKnownAvailable ? (lastKnown.protocol || "Unknown") : "—";
  if (elements.protocolLastKnownCode) elements.protocolLastKnownCode.textContent = lastKnownAvailable ? (lastKnown.code || "—") : "—";
  if (elements.protocolLastKnownSource) {
    const radio = Number(lastKnown.radio_id || 0);
    const frequency = Number(lastKnown.frequency_mhz || 0);
    elements.protocolLastKnownSource.textContent = lastKnownAvailable
      ? `${radio ? `RF${radio}` : "—"}${frequency > 0 ? ` · ${frequency.toFixed(4)} MHz` : ""}`
      : "—";
  }
  if (elements.protocolLastKnownAge) elements.protocolLastKnownAge.textContent = lastKnownAvailable ? formatAge(Number(lastKnown.age_ms || 0)) : "—";

  if (elements.protocolDiagRegistry) elements.protocolDiagRegistry.textContent = `${Number(p.evaluated_decoders || 0)} / ${Number(p.registered_decoders || 0)} evaluated`;
  if (elements.protocolDiagCounts) elements.protocolDiagCounts.textContent = `${Number(p.v2_matches || 0)} / ${Number(p.v2_no_matches || 0)}`;

  const engineDecision = p.v2_decision || "UNKNOWN";
  const engineCandidates = Array.isArray(p.v2_candidates) ? p.v2_candidates : [];
  if (elements.protocolEngineDecision) elements.protocolEngineDecision.textContent = engineDecision;
  if (elements.protocolEngineMatched) elements.protocolEngineMatched.textContent = String(Number(p.v2_candidate_count || 0));
  if (elements.protocolEngineSelected) {
    elements.protocolEngineSelected.textContent = engineDecision === "KNOWN"
      ? (p.v2_selected_protocol || "Unknown")
      : "—";
  }
  if (elements.protocolEngineCandidates) {
    elements.protocolEngineCandidates.textContent = engineCandidates.length ? engineCandidates.join(" · ") : "—";
  }
  const decisionCounts = p.v2_decision_counts || {};
  if (elements.protocolEngineUnknownCount) elements.protocolEngineUnknownCount.textContent = String(Number(decisionCounts.unknown || 0));
  if (elements.protocolEngineKnownCount) elements.protocolEngineKnownCount.textContent = String(Number(decisionCounts.known || 0));
  if (elements.protocolEngineAmbiguousCount) elements.protocolEngineAmbiguousCount.textContent = String(Number(decisionCounts.ambiguous || 0));

  const normalized = p.v2_normalized_event || {};
  const normalizedAvailable = !!normalized.available;
  if (elements.protocolNormalizedAvailable) elements.protocolNormalizedAvailable.textContent = normalizedAvailable ? "YES" : "NO";
  if (elements.protocolNormalizedProtocol) elements.protocolNormalizedProtocol.textContent = normalizedAvailable ? (normalized.protocol || "Unknown") : "—";
  if (elements.protocolNormalizedCode) elements.protocolNormalizedCode.textContent = normalizedAvailable ? (normalized.code || "—") : "—";
  if (elements.protocolNormalizedSymbols) elements.protocolNormalizedSymbols.textContent = normalizedAvailable ? String(Number(normalized.symbol_count || 0)) : "—";
  if (elements.protocolNormalizedRepeats) elements.protocolNormalizedRepeats.textContent = normalizedAvailable ? String(Number(normalized.repeats || 0)) : "—";
  if (elements.protocolNormalizedRadio) elements.protocolNormalizedRadio.textContent = normalizedAvailable && normalized.radio_id ? `RF${Number(normalized.radio_id)}` : "—";
  if (elements.protocolNormalizedFrequency) elements.protocolNormalizedFrequency.textContent = normalizedAvailable ? `${Number(normalized.frequency_mhz || 0).toFixed(4)} MHz` : "—";
  if (elements.protocolNormalizedRssi) elements.protocolNormalizedRssi.textContent = normalizedAvailable ? `${Number(normalized.rssi_dbm ?? -127).toFixed(1)} dBm` : "—";
  if (elements.protocolNormalizedRaw) elements.protocolNormalizedRaw.textContent = normalizedAvailable ? `${Number(normalized.raw_pulses || 0)} pulses · ${(Number(normalized.raw_duration_us || 0) / 1000).toFixed(2)} ms` : "—";
  if (elements.protocolNormalizedCount) elements.protocolNormalizedCount.textContent = String(Number(normalized.count || 0));

  const dedup = p.v2_event_dedup || {};
  const dedupState = dedup.state || "N/A";
  if (elements.protocolDedupState) elements.protocolDedupState.textContent = dedupState;
  if (elements.protocolDedupWindow) elements.protocolDedupWindow.textContent = `${Number(dedup.window_ms || 300)} ms`;
  if (elements.protocolDedupDelta) elements.protocolDedupDelta.textContent = dedupState === "COLLAPSED" ? `${Number(dedup.delta_ms || 0)} ms` : "—";
  if (elements.protocolDedupBurst) elements.protocolDedupBurst.textContent = String(Number(dedup.collapsed_in_burst || 0));
  if (elements.protocolDedupInputCount) elements.protocolDedupInputCount.textContent = String(Number(dedup.normalized_input_count || 0));
  if (elements.protocolDedupLogicalCount) elements.protocolDedupLogicalCount.textContent = String(Number(dedup.logical_event_count || 0));
  if (elements.protocolDedupCollapsedCount) elements.protocolDedupCollapsedCount.textContent = String(Number(dedup.collapsed_count || 0));

  const nvkp = p.v2_nvkp01 || {};
  const nvkpAvailable = !!nvkp.available;
  if (elements.protocolNvkpResult) elements.protocolNvkpResult.textContent = nvkpAvailable ? (nvkp.result || "—") : "—";
  if (elements.protocolNvkpReject) elements.protocolNvkpReject.textContent = nvkpAvailable ? (nvkp.reject_reason || "—") : "—";
  if (elements.protocolNvkpMarkers) elements.protocolNvkpMarkers.textContent = nvkpAvailable ? String(Number(nvkp.marker_pairs || 0)) : "—";
  if (elements.protocolNvkpSync) elements.protocolNvkpSync.textContent = nvkpAvailable ? String(Number(nvkp.sync_pulses || 0)) : "—";
  if (elements.protocolNvkpLeader) elements.protocolNvkpLeader.textContent = nvkpAvailable ? (nvkp.full_leader ? "YES" : "NO") : "—";
  if (elements.protocolNvkpEnvelope) elements.protocolNvkpEnvelope.textContent = nvkpAvailable ? `${Number(nvkp.pulse_count || 0)} pulses · ${(Number(nvkp.duration_us || 0) / 1000).toFixed(2)} ms` : "—";
  if (elements.protocolNvkpAlternating) elements.protocolNvkpAlternating.textContent = nvkpAvailable ? (nvkp.alternating ? "PASS" : "FAIL") : "—";
  if (elements.protocolNvkpCode) elements.protocolNvkpCode.textContent = nvkpAvailable && nvkp.code_available ? (nvkp.code || "—") : "—";
  if (elements.protocolNvkpRepeats) elements.protocolNvkpRepeats.textContent = nvkpAvailable ? String(Number(nvkp.repeats || 0)) : "—";
  if (elements.protocolNvkpRejectCounts) {
    const c = nvkp.reject_counts || {};
    elements.protocolNvkpRejectCounts.textContent = `${Number(c.capture_envelope || 0)} / ${Number(c.polarity_sequence || 0)} / ${Number(c.marker_structure || 0)}`;
  }



  const ht12e = p.v2_ht12e || {};
  const ht12eAvailable = !!ht12e.available;
  if (elements.protocolHt12eResult) elements.protocolHt12eResult.textContent = ht12eAvailable ? (ht12e.result || "—") : "—";
  if (elements.protocolHt12eReject) elements.protocolHt12eReject.textContent = ht12eAvailable ? (ht12e.reject_reason || "—") : "—";
  if (elements.protocolHt12eWords) elements.protocolHt12eWords.textContent = ht12eAvailable ? `${Number(ht12e.matching_words || 0)} / ${Number(ht12e.valid_words || 0)} / ${Number(ht12e.candidate_words || 0)}` : "—";
  if (elements.protocolHt12eT) elements.protocolHt12eT.textContent = ht12eAvailable && Number(ht12e.estimated_t_us || 0) ? `${Number(ht12e.estimated_t_us)} µs` : "—";
  if (elements.protocolHt12eTiming) elements.protocolHt12eTiming.textContent = ht12eAvailable ? `pilot ${Number(ht12e.pilot_min_us || 0)}–${Number(ht12e.pilot_max_us || 0)} µs · short ${Number(ht12e.short_min_us || 0)}–${Number(ht12e.short_max_us || 0)} µs · long ${Number(ht12e.long_min_us || 0)}–${Number(ht12e.long_max_us || 0)} µs` : "—";
  if (elements.protocolHt12eCode) elements.protocolHt12eCode.textContent = ht12eAvailable && ht12e.code_available ? (ht12e.code || "—") : "—";
  if (elements.protocolHt12eAddressData) elements.protocolHt12eAddressData.textContent = ht12eAvailable && ht12e.code_available ? `${Number(ht12e.address || 0)} / ${Number(ht12e.data || 0)}` : "—";
  if (elements.protocolHt12eRepeats) elements.protocolHt12eRepeats.textContent = ht12eAvailable ? String(Number(ht12e.repeats || 0)) : "—";
  if (elements.protocolHt12eRejectCounts) {
    const c = ht12e.reject_counts || {};
    elements.protocolHt12eRejectCounts.textContent = `${Number(c.capture_envelope || 0)} / ${Number(c.polarity_sequence || 0)} / ${Number(c.pilot_sync || 0)} / ${Number(c.symbol_timing || 0)} / ${Number(c.repeat_mismatch || 0)}`;
  }



  const ev = p.ev1527 || {};
  const allowed = ev.allowed || {};
  const evAvailable = !!ev.available;
  const textOrDash = (value) => (value === undefined || value === null || value === "") ? "—" : String(value);
  if (elements.protocolEvRejectReason) elements.protocolEvRejectReason.textContent = evAvailable ? textOrDash(ev.reject_reason) : "—";
  if (elements.protocolEvDecodedCode) elements.protocolEvDecodedCode.textContent = evAvailable ? (ev.decoded_code || "—") : "—";
  if (elements.protocolEvFrames) {
    elements.protocolEvFrames.textContent = evAvailable
      ? `${Number(ev.valid_frames || 0)} / ${Number(ev.candidate_frames || 0)} valid · ${Number(allowed.bits_per_frame || 24)} bits expected`
      : "—";
  }
  if (elements.protocolEvFrameIndexes) {
    const firstValid = Number(ev.first_valid_frame ?? -1);
    const firstFail = Number(ev.first_failing_frame ?? -1);
    elements.protocolEvFrameIndexes.textContent = evAvailable
      ? `first valid ${firstValid >= 0 ? firstValid : "—"} · first fail ${firstFail >= 0 ? firstFail : "—"}`
      : "—";
  }
  if (elements.protocolEvAddressCheck) elements.protocolEvAddressCheck.textContent = evAvailable ? textOrDash(ev.address_check) : "—";
  if (elements.protocolEvCommandCheck) elements.protocolEvCommandCheck.textContent = evAvailable ? textOrDash(ev.command_check) : "—";
  if (elements.protocolEvBaseT) {
    const base = Number(ev.base_t_us || 0);
    elements.protocolEvBaseT.textContent = evAvailable && base > 0
      ? `${base} µs · allowed ${Number(allowed.base_t_min_us || 0)}–${Number(allowed.base_t_max_us || 0)} µs`
      : "—";
  }
  if (elements.protocolEvShort) {
    elements.protocolEvShort.textContent = evAvailable && ev.short_range_available
      ? `${Number(ev.short_min_us || 0)}–${Number(ev.short_max_us || 0)} µs · allowed ±${Number(allowed.short_tolerance_pct || 0)}%`
      : "—";
  }
  if (elements.protocolEvLong) {
    elements.protocolEvLong.textContent = evAvailable && ev.short_range_available
      ? `${Number(ev.long_min_us || 0)}–${Number(ev.long_max_us || 0)} µs · allowed ±${Number(allowed.long_tolerance_pct || 0)}% around 3T`
      : "—";
  }
  if (elements.protocolEvRatio) {
    elements.protocolEvRatio.textContent = evAvailable && ev.short_range_available
      ? `${Number(ev.ratio_min || 0).toFixed(2)}–${Number(ev.ratio_max || 0).toFixed(2)} · allowed ${Number(allowed.ratio_min || 0).toFixed(2)}–${Number(allowed.ratio_max || 0).toFixed(2)}`
      : "—";
  }
  if (elements.protocolEvSync) {
    elements.protocolEvSync.textContent = evAvailable && ev.sync_low_available
      ? `${Number(ev.sync_low_t || 0).toFixed(2)}T · allowed ${Number(allowed.sync_low_t_min || 0).toFixed(2)}–${Number(allowed.sync_low_t_max || 0).toFixed(2)}T`
      : "—";
  }
  if (elements.protocolEvRepeats) {
    elements.protocolEvRepeats.textContent = evAvailable
      ? `${Number(ev.matching_repeats || 0)} / ${Number(ev.repeat_frames || 0)} matching`
      : "—";
  }
  if (elements.protocolEvRepeatDeviation) {
    elements.protocolEvRepeatDeviation.textContent = evAvailable
      ? `${Number(ev.repeat_deviation_pct || 0).toFixed(1)}% · allowed ≤${Number(allowed.repeat_deviation_max_pct || 0)}%`
      : "—";
  }
  if (elements.protocolEvRejectCounts) {
    const rejectCounts = ev.reject_counts || {};
    const nonZero = Object.entries(rejectCounts).filter(([, value]) => Number(value || 0) > 0);
    elements.protocolEvRejectCounts.textContent = nonZero.length
      ? nonZero.map(([reason, count]) => `${reason}: ${Number(count)}`).join(" · ")
      : "No rejects recorded yet";
  }

  const pt = p.pt2262 || {};
  const ptAllowed = pt.allowed || {};
  const ptAvailable = !!pt.available;
  if (elements.protocolPtResult) elements.protocolPtResult.textContent = ptAvailable ? (pt.result || "—") : "—";
  if (elements.protocolPtRejectReason) elements.protocolPtRejectReason.textContent = ptAvailable ? (pt.reject_reason || "—") : "—";
  if (elements.protocolPtDecodedCode) elements.protocolPtDecodedCode.textContent = ptAvailable ? (pt.decoded_code || "—") : "—";
  if (elements.protocolPtFrames) {
    elements.protocolPtFrames.textContent = ptAvailable
      ? `${Number(pt.valid_frames || 0)} / ${Number(pt.candidate_frames || 0)} valid · ${Number(ptAllowed.trits_per_frame || 12)} trits expected`
      : "—";
  }
  if (elements.protocolPtFrameIndexes) {
    const firstValid = Number(pt.first_valid_frame ?? -1);
    const firstFail = Number(pt.first_failing_frame ?? -1);
    elements.protocolPtFrameIndexes.textContent = ptAvailable
      ? `first valid ${firstValid >= 0 ? firstValid : "—"} · first fail ${firstFail >= 0 ? firstFail : "—"}`
      : "—";
  }
  if (elements.protocolPtTrits) {
    elements.protocolPtTrits.textContent = ptAvailable && Number(pt.decoded_trits || 0) > 0
      ? `${Number(pt.decoded_trits)} total · 0:${Number(pt.zero_trits || 0)} · 1:${Number(pt.one_trits || 0)} · F:${Number(pt.floating_trits || 0)}`
      : "—";
  }
  if (elements.protocolPtBaseT) {
    const base = Number(pt.base_t_us || 0);
    elements.protocolPtBaseT.textContent = ptAvailable && base > 0
      ? `${base} µs · allowed ${Number(ptAllowed.base_t_min_us || 0)}–${Number(ptAllowed.base_t_max_us || 0)} µs`
      : "—";
  }
  if (elements.protocolPtShort) {
    elements.protocolPtShort.textContent = ptAvailable && pt.pulse_range_available
      ? `${Number(pt.short_min_us || 0)}–${Number(pt.short_max_us || 0)} µs · class tolerance ±${Number(ptAllowed.classification_tolerance_pct || 0)}%`
      : "—";
  }
  if (elements.protocolPtLong) {
    elements.protocolPtLong.textContent = ptAvailable && pt.pulse_range_available
      ? `${Number(pt.long_min_us || 0)}–${Number(pt.long_max_us || 0)} µs`
      : "—";
  }
  if (elements.protocolPtRatio) {
    elements.protocolPtRatio.textContent = ptAvailable && pt.pulse_range_available
      ? `${Number(pt.ratio_min || 0).toFixed(2)}–${Number(pt.ratio_max || 0).toFixed(2)} · allowed ${Number(ptAllowed.ratio_min || 0).toFixed(2)}–${Number(ptAllowed.ratio_max || 0).toFixed(2)}`
      : "—";
  }
  if (elements.protocolPtSync) {
    elements.protocolPtSync.textContent = ptAvailable && pt.sync_low_available
      ? `${Number(pt.sync_low_t || 0).toFixed(2)}T · allowed ${Number(ptAllowed.sync_low_t_min || 0).toFixed(2)}–${Number(ptAllowed.sync_low_t_max || 0).toFixed(2)}T`
      : "—";
  }
  if (elements.protocolPtRepeats) {
    elements.protocolPtRepeats.textContent = ptAvailable
      ? `${Number(pt.matching_repeats || 0)} / ${Number(pt.repeat_frames || 0)} matching`
      : "—";
  }
  if (elements.protocolPtRepeatDeviation) {
    elements.protocolPtRepeatDeviation.textContent = ptAvailable
      ? `${Number(pt.repeat_deviation_pct || 0).toFixed(1)}% · allowed ≤${Number(ptAllowed.repeat_deviation_max_pct || 0)}%`
      : "—";
  }
  if (elements.protocolPtRejectCounts) {
    const rejectCounts = pt.reject_counts || {};
    const nonZero = Object.entries(rejectCounts).filter(([, value]) => Number(value || 0) > 0);
    elements.protocolPtRejectCounts.textContent = nonZero.length
      ? nonZero.map(([reason, count]) => `${reason}: ${Number(count)}`).join(" · ")
      : "No rejects recorded yet";
  }
}

async function loadRadioStatus(force = false) {
  if (radioStatusRequestActive && !force) return;
  // A user action such as Restore Default must not be hidden by a background
  // poll that happens to be in progress.
  if (force) radioStatusRequestActive = false;
  radioStatusRequestActive = true;
  try {
    const d = await requestJson("/api/radio");
    const frequency = Number(d.frequency_mhz);
    const rssi = Number(d.rssi_dbm);
    elements.radioChip.textContent = d.dual_radio ? "2 × CC1101" : (d.chip || "-");
    elements.radioMode.textContent = d.mode || "-";
    const rf1Operating = Number(d.rf1_operating_frequency_mhz);
    const rf2Operating = Number(d.rf2_operating_frequency_mhz);
    elements.radioFrequency.textContent = d.dual_radio
      ? `${rf1Operating.toFixed(3)} + ${rf2Operating.toFixed(3)} MHz`
      : (Number.isFinite(frequency) && frequency > 0 ? `${frequency.toFixed(3)} MHz` : "-");
    elements.radioRssi.textContent = d.dual_radio
      ? `433: ${Number(d.rf1_rssi_dbm).toFixed(1)} · 868: ${Number(d.rf2_rssi_dbm).toFixed(1)} dBm`
      : (Number.isFinite(rssi) ? `${rssi.toFixed(1)} dBm` : "-");
    const r1c = d.radio1_capture || {};
    const r2c = d.radio2_capture || {};
    if (elements.rf1DiagEdges) elements.rf1DiagEdges.textContent = r1c.edges ?? "—";
    if (elements.rf1DiagCandidates) elements.rf1DiagCandidates.textContent = r1c.raw_candidates ?? "—";
    if (elements.rf1DiagAccepted) elements.rf1DiagAccepted.textContent = r1c.accepted_frames ?? "—";
    if (elements.rf1DiagRejected) elements.rf1DiagRejected.textContent = r1c.rejected_frames ?? "—";
    if (elements.rf1DiagPulses) elements.rf1DiagPulses.textContent = r1c.current_pulses ?? "—";
    if (elements.rf2DiagEdges) elements.rf2DiagEdges.textContent = r2c.edges ?? "—";
    if (elements.rf2DiagCandidates) elements.rf2DiagCandidates.textContent = r2c.raw_candidates ?? "—";
    if (elements.rf2DiagAccepted) elements.rf2DiagAccepted.textContent = r2c.accepted_frames ?? "—";
    if (elements.rf2DiagRejected) elements.rf2DiagRejected.textContent = r2c.rejected_frames ?? "—";
    if (elements.rf2DiagPulses) elements.rf2DiagPulses.textContent = r2c.current_pulses ?? "—";

    renderCaptureFinalizationDiagnostics(1, r1c);
    renderCaptureFinalizationDiagnostics(2, r2c);
    renderProtocolDiagnostics(d);
  } catch (error) {
    // Preserve the last good System diagnostics during a transient API/Wi-Fi
    // interruption instead of making the counters appear to reset.
    console.error(error);
  } finally {
    radioStatusRequestActive = false;
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
      renderRfHardwareStatus(d);
      renderDashboardRadios(d);
      setGauge(elements.core0Gauge, elements.core0Load, d.core0_load_percent);
      setGauge(elements.core1Gauge, elements.core1Load, d.core1_load_percent);
      setGauge(elements.psramGauge, elements.psramLoad, d.psram_used_percent);
      setGauge(elements.heapGauge, elements.heapLoad, d.heap_used_percent);
      renderMemoryDiagnostics(d);
      renderFirmwareVersion(d);
      elements.ip.textContent = d.ip || "-";
      elements.wifiMode.textContent = d.wifi_mode || "-";
      elements.mqttStatus.textContent = d.mqtt_state || (d.mqtt_enabled ? "Disconnected" : "Disabled");
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
let analyzerRenderedRadioId = 0;
let analyzerLastFullFetchMs = 0;
let analyzerPendingFullRefresh = false;
let analyzerCandidateRefreshTimer = null;
let analyzerViewMode = "both";
let analyzerSelectedRadio = 1;
const ANALYZER_MIN_FULL_REFRESH_MS = 250;
const ANALYZER_CANDIDATE_DEBOUNCE_MS = 350;


function analyzerRadioQuery() {
  return analyzerSelectedRadio === 2 ? 2 : 1;
}

function clearAnalyzerDetail(radioId) {
  if (elements.analyzerSequence) elements.analyzerSequence.textContent = `Radio ${radioId} · waiting for signal`;
  if (elements.analyzerBadge) {
    elements.analyzerBadge.textContent = "Idle";
    elements.analyzerBadge.className = "badge badge-loading";
  }
  if (elements.candidateSequence) elements.candidateSequence.textContent = "Waiting for candidate";
  if (elements.candidateAge) elements.candidateAge.textContent = "-";
  if (elements.candidateRssi) elements.candidateRssi.textContent = "-";
  if (elements.candidatePulseCount) elements.candidatePulseCount.textContent = "-";
  if (elements.candidateDuration) elements.candidateDuration.textContent = "-";
  if (elements.candidateRejectReason) elements.candidateRejectReason.textContent = "-";
  if (elements.candidateRaw) elements.candidateRaw.textContent = `Radio ${radioId}: no Candidate received yet.`;
  if (elements.analyzerFrequency) elements.analyzerFrequency.textContent = "-";
  if (elements.analyzerRssi) elements.analyzerRssi.textContent = "-";
  if (elements.analyzerProtocol) elements.analyzerProtocol.textContent = "-";
  if (elements.analyzerEncoding) elements.analyzerEncoding.textContent = "-";
  if (elements.analyzerBits) elements.analyzerBits.textContent = "-";
  if (elements.analyzerBasePulse) elements.analyzerBasePulse.textContent = "-";
  if (elements.analyzerFrameCount) elements.analyzerFrameCount.textContent = "-";
  if (elements.analyzerQuality) elements.analyzerQuality.textContent = "-";
  if (elements.analyzerPulseCount) elements.analyzerPulseCount.textContent = "-";
  if (elements.analyzerDuration) elements.analyzerDuration.textContent = "-";
  if (elements.analyzerCode) elements.analyzerCode.textContent = "-";
  if (elements.analyzerClasses) elements.analyzerClasses.textContent = "-";
  if (elements.analyzerDetailTitle) elements.analyzerDetailTitle.textContent = `Radio ${radioId} · no frame analyzed yet`;
  if (elements.analyzerBitstream) elements.analyzerBitstream.textContent = `Waiting for Radio ${radioId} RF traffic.`;
}

function renderAnalyzerViewButtons() {
  const page = document.getElementById("page-analyzer");
  if (page) page.classList.toggle("analyzer-both-mode", analyzerViewMode === "both");

  const buttons = [
    [elements.analyzerViewBoth, "both"],
    [elements.analyzerViewRadio1, "1"],
    [elements.analyzerViewRadio2, "2"]
  ];

  buttons.forEach(([button, mode]) => {
    if (!button) return;
    const active = analyzerViewMode === mode;
    button.classList.toggle("active", active);
    button.classList.toggle("button-primary", active);
    button.classList.toggle("button-secondary", !active);
  });
}

async function setAnalyzerView(mode) {
  analyzerViewMode = mode;
  if (mode === "1" || mode === "2") {
    analyzerSelectedRadio = Number(mode);
    analyzerRenderedSequence = -1;
    analyzerRenderedCandidateSequence = -1;
    analyzerRenderedRadioId = 0;
    analyzerLastFullFetchMs = 0;
    clearAnalyzerDetail(analyzerSelectedRadio);
  } else {
    // Both is an aggregate view only. It never owns a third Analyzer state;
    // the detail report follows whichever physical radio has the newest
    // complete Analyzer result.
    analyzerRenderedSequence = -1;
    analyzerRenderedCandidateSequence = -1;
    analyzerRenderedRadioId = 0;
    analyzerLastFullFetchMs = 0;
  }

  renderAnalyzerViewButtons();

  if (mode === "both") {
    await loadAnalyzerBothSummary();
  } else {
    await loadAnalyzer();
  }
}

function renderAnalyzerSummary(radioId, d) {
  const is1 = radioId === 1;
  const badge = is1 ? elements.analyzerSummary1Badge : elements.analyzerSummary2Badge;
  const frequency = is1 ? elements.analyzerSummary1Frequency : elements.analyzerSummary2Frequency;
  const candidate = is1 ? elements.analyzerSummary1Candidate : elements.analyzerSummary2Candidate;
  const frame = is1 ? elements.analyzerSummary1Frame : elements.analyzerSummary2Frame;
  const rssi = is1 ? elements.analyzerSummary1Rssi : elements.analyzerSummary2Rssi;

  if (frequency) frequency.textContent = `${Number(d.frequency_mhz || 0).toFixed(4)} MHz`;
  if (candidate) candidate.textContent = d.candidate_available ? `#${Number(d.candidate_sequence || 0)}` : "Waiting";
  if (frame) frame.textContent = d.available ? `#${Number(d.sequence || 0)}` : "Waiting";
  if (rssi) rssi.textContent = Number(d.peak_rssi_dbm) > -127 ? `${Number(d.peak_rssi_dbm).toFixed(1)} dBm` : "-";

  if (badge) {
    const activity = d.available || d.candidate_available;
    badge.textContent = activity ? "ACTIVE" : "WAITING";
    badge.className = `badge ${activity ? "badge-online" : "badge-loading"}`;
  }
}

function newestAnalyzerRadio(r1, r2) {
  const a1 = !!r1?.available;
  const a2 = !!r2?.available;
  if (!a1 && !a2) return 0;
  if (a1 && !a2) return 1;
  if (a2 && !a1) return 2;

  const age1 = Number(r1.age_ms);
  const age2 = Number(r2.age_ms);
  if (!Number.isFinite(age1)) return 2;
  if (!Number.isFinite(age2)) return 1;
  return age2 < age1 ? 2 : 1;
}

async function loadAnalyzerBothSummary() {
  if (analyzerLiveRequestActive) return;
  analyzerLiveRequestActive = true;
  try {
    const [r1, r2] = await Promise.all([
      requestJson("/api/analyzer/live?radio=1"),
      requestJson("/api/analyzer/live?radio=2")
    ]);
    renderAnalyzerSummary(1, r1);
    renderAnalyzerSummary(2, r2);

    const latestRadio = newestAnalyzerRadio(r1, r2);
    if (!latestRadio) {
      analyzerRenderedRadioId = 0;
      analyzerRenderedSequence = -1;
      if (elements.analyzerSequence) elements.analyzerSequence.textContent = "Both radios · waiting for analyzed frame";
      if (elements.analyzerBadge) {
        elements.analyzerBadge.textContent = "Idle";
        elements.analyzerBadge.className = "badge badge-loading";
      }
      if (elements.analyzerDetailTitle) elements.analyzerDetailTitle.textContent = "No frame analyzed yet";
      if (elements.analyzerAge) elements.analyzerAge.textContent = "-";
      if (elements.analyzerBitstream) elements.analyzerBitstream.textContent = "Waiting for an accepted/structured frame from Radio 1 or Radio 2.";
      return;
    }

    const latestLive = latestRadio === 2 ? r2 : r1;
    const latestSequence = Number(latestLive.sequence || 0);
    if (analyzerRenderedRadioId !== latestRadio || analyzerRenderedSequence !== latestSequence) {
      await loadAnalyzer(latestRadio);
    } else if (elements.analyzerAge) {
      elements.analyzerAge.textContent = `${Math.round(Number(latestLive.age_ms || 0) / 1000)} s ago`;
    }
  } catch (_) {
  } finally {
    analyzerLiveRequestActive = false;
  }
}

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
  if (analyzerViewMode === "both") await loadAnalyzerBothSummary();
  else await loadAnalyzer();
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

  if (analyzerViewMode === "both") {
    await loadAnalyzerBothSummary();
    return;
  }

  analyzerLiveRequestActive = true;
  try {
    const radioId = analyzerRadioQuery();
    const d = await requestJson(`/api/analyzer/live?radio=${radioId}`);
    if (!d.enabled) return;

    if (elements.analyzerPeakRssi && Number(d.peak_rssi_dbm) > -127) {
      elements.analyzerPeakRssi.textContent = `${Number(d.peak_rssi_dbm).toFixed(1)} dBm`;
    }
    if (elements.analyzerWeakRssi) {
      elements.analyzerWeakRssi.textContent = d.weak_rssi_frames || 0;
    }
    if (elements.analyzerCurrentRssi && Number.isFinite(Number(d.current_rssi_dbm))) {
      elements.analyzerCurrentRssi.textContent = `${Number(d.current_rssi_dbm).toFixed(1)} dBm`;
    }

    const seq = Number(d.sequence || 0);
    const candidateSeq = Number(d.candidate_sequence || 0);

    if (seq !== analyzerRenderedSequence) {
      analyzerPendingFullRefresh = true;
      loadAnalyzerLatest();
    } else if (candidateSeq !== analyzerRenderedCandidateSequence) {
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

async function loadAnalyzer(radioOverride = 0) {
  if (!elements.analyzerSequence || analyzerRequestActive) return;
  analyzerRequestActive = true;
  try {
    const radioId = radioOverride === 2 ? 2 : (radioOverride === 1 ? 1 : analyzerRadioQuery());
    const analyzerRequestStarted = performance.now();
    const d = await requestJson(`/api/analyzer?radio=${radioId}`);
    const analyzerRequestMs = performance.now() - analyzerRequestStarted;
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
    const candidateLatencyAgeMs = Number(d.last_candidate?.age_ms || 0);
    const candidateLatencySequence = Number(d.last_candidate?.sequence || 0);
    if (elements.analyzerServerAge) {
      elements.analyzerServerAge.textContent =
        d.available ? `${Number(d.age_ms || 0).toFixed(0)} ms` : "no result";
    }
    if (elements.analyzerCandidateLatencyAge) {
      elements.analyzerCandidateLatencyAge.textContent =
        d.last_candidate?.available ? `${candidateLatencyAgeMs.toFixed(0)} ms` : "no candidate";
    }
    if (elements.analyzerProcessingTime) elements.analyzerProcessingTime.textContent = `${(Number(d.processing_us || 0) / 1000).toFixed(2)} ms`;
    if (elements.analyzerApiBuildTime) elements.analyzerApiBuildTime.textContent = `${(Number(d.api_build_us || 0) / 1000).toFixed(2)} ms`;
    if (elements.analyzerBrowserRequestTime) elements.analyzerBrowserRequestTime.textContent = `${analyzerRequestMs.toFixed(0)} ms`;
    if (elements.analyzerLatencyState) {
      elements.analyzerLatencyState.textContent =
        `R${radioId} · result ${Number(d.sequence || 0)} · cand ${candidateLatencySequence}`;
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
    analyzerRenderedRadioId = radioId;
    analyzerRenderedSequence = Number(d.sequence || 0);
    const c = d.last_candidate || {};
    analyzerRenderedCandidateSequence = Number(c.sequence || 0);
    const analyzerRssiLimit = Number(d.analyzer_min_rssi ?? -75);
    const candidateRssi = Number(c.rssi_dbm ?? -127);
    const candidateFilteredByRssi =
      !!c.available && Number.isFinite(candidateRssi) &&
      Number.isFinite(analyzerRssiLimit) && candidateRssi < analyzerRssiLimit;
    const candidateIsNewerThanAnalyzedFrame =
      !d.available ||
      Number(c.age_ms ?? Number.MAX_SAFE_INTEGER) <=
        Number(d.age_ms ?? Number.MAX_SAFE_INTEGER);
    const latestFrameFilteredByRssi =
      candidateFilteredByRssi && candidateIsNewerThanAnalyzedFrame;
    if (c.available) {
      elements.candidateSequence.textContent = `Radio ${radioId} · Candidate #${c.sequence}`;
      elements.candidateAge.textContent = formatAge(Number(c.age_ms));
      elements.candidateRssi.textContent = `${Number(c.rssi_dbm).toFixed(1)} dBm`;
      elements.candidatePulseCount.textContent = `${c.pulse_count || 0} pulses`;
      elements.candidateDuration.textContent = `${(Number(c.duration_us || 0) / 1000).toFixed(2)} ms`;
      elements.candidateRejectReason.textContent = candidateFilteredByRssi
        ? `${c.reject_reason || "received"} · Analyzer RSSI filtered (${candidateRssi.toFixed(1)} < ${analyzerRssiLimit.toFixed(0)} dBm)`
        : (c.reject_reason || "-");
      elements.candidateAlternation.textContent = `${c.alternation_ratio || 0}%`;
      elements.candidateSamePairs.textContent = String(c.same_sign_pairs || 0);
      elements.candidateLongestRun.textContent = String(c.longest_same_sign_run || 0);
      elements.candidateNormalizedCount.textContent = `${c.normalized_pulse_count || 0} pulses`;
      const normalizedRaw = Array.isArray(c.normalized_pulses_us) ? c.normalized_pulses_us : [];
      elements.candidateNormalizedRaw.textContent = `Normalized signed pulse sequence (µs):\n${normalizedRaw.join(", ")}`;
      const candidateRaw = Array.isArray(c.raw_pulses_us) ? c.raw_pulses_us : [];
      elements.candidateRaw.textContent = `Source: Radio ${Number(c.radio_id || radioId)}
Frequency: ${Number(c.frequency_mhz).toFixed(4)} MHz
Pulse min / max: ${c.min_pulse_us || 0} / ${c.max_pulse_us || 0} µs
RAW signed pulse sequence (µs):
${candidateRaw.join(", ")}${c.raw_truncated ? `
… preview truncated after ${candidateRaw.length} pulses` : ""}`;
    }
    if (latestFrameFilteredByRssi) {
        elements.analyzerSequence.textContent = `Radio ${radioId} · Candidate #${Number(c.sequence || 0)}`;
        elements.analyzerBadge.textContent = "RSSI FILTERED";
        elements.analyzerBadge.className = "badge badge-loading";
        elements.analyzerFrequency.textContent = `${Number(c.frequency_mhz || 0).toFixed(4)} MHz · R${radioId}`;
        elements.analyzerRssi.textContent = `${candidateRssi.toFixed(1)} dBm`;
        elements.analyzerPulseCount.textContent = `${Number(c.pulse_count || 0)} pulses`;
        elements.analyzerDuration.textContent = `${(Number(c.duration_us || 0) / 1000).toFixed(2)} ms`;
        elements.analyzerDetailTitle.textContent =
          `Radio ${radioId} · frame received, filtered by Analyzer RSSI`;
        elements.analyzerBitstream.textContent =
          `Frame received and captured successfully.\nAnalyzer RSSI: ${candidateRssi.toFixed(1)} dBm\nConfigured minimum: ${analyzerRssiLimit.toFixed(0)} dBm\nLower the Analyzer minimum RSSI to inspect this frame. Gateway and V2 Protocol Engine operation are unaffected.`;
        if (elements.analyzerModeMessage) {
          elements.analyzerModeMessage.innerHTML =
            `<strong>Frame received, but filtered by the Analyzer RSSI threshold.</strong> Signal ${candidateRssi.toFixed(1)} dBm · limit ${analyzerRssiLimit.toFixed(0)} dBm. The gateway and V2 Protocol Engine remain fully operational.`;
        }
      return;
    }
    if (!d.available) {
      elements.analyzerDetailTitle.textContent = `Radio ${radioId} · no analyzed frame yet`;
      elements.analyzerBitstream.textContent = `Waiting for an accepted/structured Radio ${radioId} frame.`;
      return;
    }
    elements.analyzerSequence.textContent = `Radio ${radioId} · Frame #${d.sequence}`;
    elements.analyzerBadge.textContent = d.status || "ACTIVE";
    elements.analyzerBadge.className = `badge ${d.accepted ? "badge-online" : "badge-loading"}`;
    if (elements.analyzerModeMessage) elements.analyzerModeMessage.innerHTML = "<strong>RF Analyzer is running.</strong> Capturing RF traffic in the background. On ESP32-S3, Analyzer runs in non-exclusive mode while the gateway remains fully operational.";
    elements.analyzerFrequency.textContent = `${Number(d.frequency_mhz).toFixed(4)} MHz · R${radioId}`;
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
    elements.analyzerDetailTitle.textContent = `Radio ${radioId} · ${Number(d.frequency_mhz).toFixed(4)} MHz · ${d.protocol || "Unknown"} · ${reason}`;
    const raw = Array.isArray(d.raw_pulses_us) ? d.raw_pulses_us : [];
    const rawText = raw.length ? raw.join(", ") + (d.raw_truncated ? `\n… preview truncated after ${raw.length} pulses` : "") : "No RAW pulse data.";
    const decodedText = d.bitstream ? `Decoded bitstream: ${d.bitstream}\n\n` : "";
    const structuredText = d.structured_signal ? `Structured signal: yes\nOccurrences: ${d.occurrences || 0}\nSimilarity: ${d.similarity || 0}%\n\n` : "";
    elements.analyzerBitstream.textContent = `${decodedText}${structuredText}Source: Radio ${radioId}
Frequency: ${Number(d.frequency_mhz).toFixed(4)} MHz
Protocol: ${d.protocol || "Unknown"}
Encoding guess: ${d.encoding || "Unknown"}
Status: ${reason}
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
  else if (state === "ACCEPTED_RAM") { elements.learnBadge.className = "badge badge-online"; elements.learnBadge.textContent = "Preview kept"; elements.learnMessage.textContent = "Preview kept temporarily. Save to Slot to retain it after restart."; }
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
  const card = document.createElement("article"); card.className = `card slot-card${slot.used ? "" : " empty"}`; card.dataset.rawSlotId = String(slot.id);
  const title = document.createElement("div"); title.className = "slot-title";
  const heading = document.createElement("div"); heading.innerHTML = `<span class="slot-number">Slot ${slot.id}</span><h3></h3>`; heading.querySelector("h3").textContent = slot.name || `RF Slot ${slot.id}`;
  const badge = document.createElement("span"); badge.className = `badge ${slot.used ? "badge-online" : "badge-loading"}`; badge.textContent = slot.used ? "TX + RX" : "Empty";
  title.append(heading, badge); card.append(title);
  if (slot.used) {
    const learnedRadio = Number(slot.radio_id || 0);
    const radioText = learnedRadio === 1 ? "Radio 1" : (learnedRadio === 2 ? "Radio 2" : "Legacy / unspecified");
    const tunedText = slot.frequency_tuned ? " · TUNED" : "";
    const meta = document.createElement("div"); meta.className = "slot-meta";
    meta.innerHTML = `<span>Pulses<strong>${slot.pulse_count}</strong></span><span>Duration<strong>${(Number(slot.duration_us) / 1000).toFixed(2)} ms</strong></span><span>Radio<strong>${radioText}</strong></span><span>Learned frequency<strong>${Number(slot.frequency_mhz).toFixed(4)} MHz${tunedText}</strong></span><span>Fingerprint<strong>${fingerprintHex(slot.fingerprint)}</strong></span><span>RX matches<strong data-raw-stat="match_count">${Number(slot.rx_match_count || 0)}</strong></span><span>Last similarity<strong data-raw-stat="last_similarity">${Number(slot.rx_last_similarity || 0).toFixed(0)}%</strong></span><span>Last RSSI<strong data-raw-stat="last_rssi">${Number(slot.rx_last_rssi ?? -127).toFixed(1)} dBm</strong></span>`;
    card.append(meta);
    const input = document.createElement("input"); input.className = "slot-name-input"; input.maxLength = 32; input.value = slot.name;
    const actions = document.createElement("div"); actions.className = "slot-actions";
    const send = document.createElement("button"); send.className = "button button-primary"; send.textContent = "Send"; send.onclick = () => slotAction("send", slot.id);
    const rename = document.createElement("button"); rename.className = "button button-secondary"; rename.textContent = "Rename"; rename.onclick = () => slotAction("rename", slot.id, input.value);
    const del = document.createElement("button"); del.className = "button button-danger"; del.textContent = "Delete"; del.onclick = () => { if (window.confirm(`Delete slot ${slot.id}?`)) slotAction("delete", slot.id); };
    actions.append(send, rename, del); card.append(input, actions);
  } else {
    const text = document.createElement("p"); text.className = "muted"; text.textContent = "Learn a signal, then save it here for RAW replay + RX matching."; card.append(text);
  }
  return card;
}
async function refreshRawSlotLiveStats() {
  if (!elements.slotGrid) return;
  try {
    const d = await requestJson("/api/slots/stats");
    (d.slots || []).forEach(slot => {
      const card = elements.slotGrid.querySelector(`[data-raw-slot-id="${Number(slot.id)}"]`);
      if (!card) return;
      const match = card.querySelector('[data-raw-stat="match_count"]');
      const similarity = card.querySelector('[data-raw-stat="last_similarity"]');
      const rssi = card.querySelector('[data-raw-stat="last_rssi"]');
      if (match) match.textContent = Number(slot.match_count || 0);
      if (similarity) similarity.textContent = `${Number(slot.last_similarity || 0).toFixed(0)}%`;
      if (rssi) rssi.textContent = `${Number(slot.last_rssi ?? -127).toFixed(1)} dBm`;
    });
  } catch (_) {
    // Keep the last visible stats during a short API interruption.
  }
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
  const card=document.createElement("article"); card.className=`card slot-card${slot.used?"":" empty"}${slot.id===learningSlot?" rx-learning":""}`; card.dataset.rxSlotId=String(slot.id);
  const title=document.createElement("div"); title.className="slot-title";
  const heading=document.createElement("div"); heading.innerHTML=`<span class="slot-number">RX Slot ${slot.id}</span><h3></h3>`; heading.querySelector("h3").textContent=slot.name||`RX Slot ${slot.id}`;
  const badge=document.createElement("span"); badge.className=`badge ${slot.id===learningSlot?"badge-loading":(slot.used&&slot.enabled?"badge-online":"badge-loading")}`; badge.textContent=slot.id===learningSlot?"Waiting":(slot.used?(slot.enabled?"Active":"Disabled"):"Empty"); title.append(heading,badge);card.append(title);
  if(slot.used){
    const learnedRadio=Number(slot.radio_id||0);const learnedFrequency=Number(slot.frequency_mhz||0);const radioText=learnedRadio===1?"Radio 1":(learnedRadio===2?"Radio 2":"Legacy / any");const frequencyText=learnedFrequency>0?`${learnedFrequency.toFixed(4)} MHz`:"Legacy / not stored";const meta=document.createElement("div");meta.className="slot-meta";meta.innerHTML=`<span>Protocol<strong>${slot.protocol||"Unknown"}</strong></span><span>Code<strong>${slot.code||"—"}</strong></span><span>Radio<strong>${radioText}</strong></span><span>Learned frequency<strong>${frequencyText}</strong></span><span>Symbols<strong>${slot.symbol_count||0}</strong></span><span>Matches<strong data-rx-stat="match_count">${slot.match_count||0}</strong></span><span>Last quality<strong data-rx-stat="last_quality">${Number(slot.last_quality||0).toFixed(0)}%</strong></span><span>Last RSSI<strong data-rx-stat="last_rssi">${Number(slot.last_rssi??-127).toFixed(1)} dBm</strong></span>`;card.append(meta);
    const input=document.createElement("input");input.className="slot-name-input";input.maxLength=32;input.value=slot.name;
    const actions=document.createElement("div");actions.className="slot-actions";
    const send=document.createElement("button");send.className="button button-primary";send.textContent="Send";send.disabled=Boolean(learningSlot)||!slot.send_supported;send.title=slot.send_supported?"Transmit this decoded RF command":"Protocol SEND is not available for this slot yet";send.onclick=()=>rxSlotAction("send",slot.id,"",false,send);
    const learn=document.createElement("button");learn.className="button button-primary";learn.textContent="Capture again";learn.disabled=Boolean(learningSlot);learn.onclick=()=>rxSlotAction("learn",slot.id,input.value, false, learn);
    const rename=document.createElement("button");rename.className="button button-secondary";rename.textContent="Rename";rename.disabled=Boolean(learningSlot);rename.onclick=()=>rxSlotAction("rename",slot.id,input.value);
    const enable=document.createElement("button");enable.className="button button-secondary";enable.textContent=slot.enabled?"Disable":"Enable";enable.disabled=Boolean(learningSlot);enable.onclick=()=>rxSlotAction("enable",slot.id,"",!slot.enabled);
    const del=document.createElement("button");del.className="button button-danger";del.textContent="Delete";del.disabled=Boolean(learningSlot);del.onclick=()=>{if(confirm(`Delete RX slot ${slot.id}?`))rxSlotAction("delete",slot.id);};actions.append(send,learn,rename,enable,del);card.append(input,actions);
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

function renderProtocolTx(d) {
    const txAvailable=Boolean(d.v2_tx_available);
    if(elements.rxV2TxResult) elements.rxV2TxResult.textContent=txAvailable?String(d.v2_tx_result||"N/A"):"N/A";
    if(elements.rxV2TxProtocol) elements.rxV2TxProtocol.textContent=txAvailable?`${d.v2_tx_protocol||"Unknown"} · ${d.v2_tx_code||"—"}`:"—";
    if(elements.rxV2TxRadio) elements.rxV2TxRadio.textContent=txAvailable?`Radio ${Number(d.v2_tx_radio||0)} · ${Number(d.v2_tx_frequency_mhz||0).toFixed(4)} MHz`:"—";
    if(elements.rxV2TxRepeats) elements.rxV2TxRepeats.textContent=txAvailable?String(d.v2_tx_repeats||0):"—";
    if(elements.rxV2TxAge) {
      const failure=String(d.v2_tx_failure||"NONE");
      elements.rxV2TxAge.textContent=txAvailable?`${Math.round(Number(d.v2_tx_age_ms||0)/1000)} s · ${failure}`:"—";
    }
}
let protocolTxRequestActive = false;
async function loadProtocolTx() {
  if (protocolTxRequestActive) return;
  protocolTxRequestActive = true;
  try { renderProtocolTx(await requestJson("/api/rxslots")); }
  catch (_) { elements.rxV2TxResult.textContent = "Unavailable · refresh pending"; }
  finally { protocolTxRequestActive = false; }
}
async function loadRxSlots({silent=false}={}) {
  if(!elements.rxSlotGrid)return null;
  if(!silent) elements.refreshRxSlotsButton.disabled=true;
  if(!silent) setRxSlotMessage("Loading...");
  try {
    const d=await requestJson("/api/rxslots");
    const learningSlot=Number(d.learning_slot||0);
    renderRxLearnRssi(d.learn_min_rssi);
    renderProtocolTx(d);
    elements.rxSlotUsage.textContent=`${d.used_count||0} of ${Number(d.count ?? (d.slots||[]).length)} RX slots used${learningSlot?` — capturing into slot ${learningSlot}`:""}`;
    elements.rxSlotGrid.replaceChildren(...(d.slots||[]).map(slot=>rxSlotCard(slot,learningSlot)));
    if(d.learn_state==="waiting_for_signal") {
      const weak=Number(d.weak_rejected||0);
      if(weak>0) setRxSlotMessage(`Waiting for a stronger signal... ${weak} weak capture${weak===1?"":"s"} ignored; last ${Number(d.last_weak_rssi||-127).toFixed(1)} dBm, minimum ${Number(d.learn_min_rssi||-75)} dBm.`);
      else setRxSlotMessage("Waiting for a supported fixed-code RF signal...");
    }
    else if(d.learn_state==="unsupported_protocol") setRxSlotMessage("Unsupported or ambiguous protocol. No RX slot was saved.","error");
    else if(d.learn_state==="duplicate_code") setRxSlotMessage("This RF code is already assigned to another RX slot.","error");
    else if(d.learn_state==="saved") {
      setRxSlotMessage("RX button captured and saved from the V2-native Known Protocol path. Home Assistant discovery update queued.","success");
    }
    else if(d.learn_state==="save_error") setRxSlotMessage("The RF signal was received, but saving failed.","error");
    else if(d.learn_state==="timeout") setRxSlotMessage("Capture timed out because no usable RF signal arrived. The receiver was released and Capture can be started again.","error");
    else if(d.learn_state==="radio_error") setRxSlotMessage("Radio could not re-arm or release the Learn session.","error");
    else if(!silent) setRxSlotMessage("");
    return d;
  } catch(e) { setRxSlotMessage(e.message,"error"); return null; }
  finally { if(!silent) elements.refreshRxSlotsButton.disabled=false; }
}

let rxSlotLiveRefreshBusy = false;
async function refreshRxSlotLiveStats() {
  if (rxSlotLiveRefreshBusy || rxLearnPollTimer || !elements.rxSlotGrid) return;
  rxSlotLiveRefreshBusy = true;
  try {
    const d = await requestJson("/api/rxslots");

    // If backend Learn state changed while the dedicated Learn poller is not
    // running (for example watchdog timeout), do one full render so buttons
    // and badges cannot stay visually stuck.
    const backendLearningSlot = Number(d.learning_slot || 0);
    const visibleLearningCard = elements.rxSlotGrid.querySelector(".rx-learning");
    if ((backendLearningSlot > 0) !== Boolean(visibleLearningCard)) {
      await loadRxSlots({silent:true});
      return;
    }

    for (const slot of (d.slots || [])) {
      if (!slot.used) continue;
      const card = elements.rxSlotGrid.querySelector(`[data-rx-slot-id="${Number(slot.id)}"]`);
      if (!card) continue;
      const matches = card.querySelector('[data-rx-stat="match_count"]');
      const quality = card.querySelector('[data-rx-stat="last_quality"]');
      const rssi = card.querySelector('[data-rx-stat="last_rssi"]');
      if (matches) matches.textContent = String(Number(slot.match_count || 0));
      if (quality) quality.textContent = `${Number(slot.last_quality || 0).toFixed(0)}%`;
      if (rssi) rssi.textContent = `${Number(slot.last_rssi ?? -127).toFixed(1)} dBm`;
    }
  } catch (_) {
    // Keep the last visible values during a short Wi-Fi/API interruption.
  } finally {
    rxSlotLiveRefreshBusy = false;
  }
}

async function pollRxLearn() {
  const data = await loadRxSlots({silent:true});
  if (!data) { stopRxLearnPolling(); return; }
  if (data.learn_state === "waiting_for_signal" && Number(data.learning_slot||0) > 0) {
    if (Date.now() >= rxLearnDeadline) { setRxSlotMessage("Capture did not finish in time. Checking receiver state...","error"); rxLearnPollTimer=window.setTimeout(pollRxLearn,650); return; }
    rxLearnPollTimer = window.setTimeout(pollRxLearn, 650);
    return;
  }
  if (data.learn_state === "saved") {
    setRxSlotMessage(`RX slot ${rxLearningSlot || ""} captured successfully via V2-native learn. Home Assistant discovery was republished.`,"success");
    stopRxLearnPolling();
    window.setTimeout(()=>loadRxSlots(),1200);
    return;
  }
  if (data.learn_state === "save_error") setRxSlotMessage("RX capture failed while saving.","error");
  else if (data.learn_state === "timeout") setRxSlotMessage("Capture timed out safely. No usable RF signal was saved and the receiver is ready again.","error");
  stopRxLearnPolling();
}

async function rxSlotAction(action,slot,name="",enabled=false,button=null) {
  setRxSlotMessage(action==="learn"?"Starting RX capture...":`${action}...`);
  if(button){button.disabled=true;button.classList.add("busy");}
  try {
    const payload={slot};if(action==="learn"||action==="rename")payload.name=name.trim();if(action==="enable")payload.enabled=enabled;
    const d=await postJson(`/api/rxslots/${action}`,payload);
    if(action==="learn") {
      rxLearningSlot=slot; rxLearnDeadline=Date.now()+50000;
      setRxSlotMessage("Waiting for RF signal. Press the remote button once...");
      await loadRxSlots({silent:true});
      rxLearnPollTimer=window.setTimeout(pollRxLearn,500);
    } else {
      setRxSlotMessage(d.message,"success"); await loadRxSlots();
    }
  } catch(e) { setRxSlotMessage(e.message,"error"); stopRxLearnPolling(); }
  finally { if(button){button.classList.remove("busy"); if(action!=="learn")button.disabled=false;} }
}

async function loadConfig() { setSaveMessage("Loading settings..."); elements.saveButton.disabled = true; try { const d = await requestJson("/api/config"); elements.hostname.value = d.hostname || "OpenRF-Platform"; elements.replayCount.value = Number(d.replay_count) >= 1 ? d.replay_count : 1; elements.wifiSsid.value = d.wifi_ssid || ""; elements.wifiPassword.value = ""; elements.wifiPasswordState.textContent = d.wifi_password_set ? "A WiFi password is saved. Leave empty to keep it." : "No WiFi password is saved."; elements.mqttEnabled.checked = Boolean(d.mqtt_enabled); elements.mqttHost.value = d.mqtt_host || ""; elements.mqttPort.value = d.mqtt_port || 1883; elements.mqttUser.value = d.mqtt_user || ""; elements.mqttPassword.value = ""; elements.homeAssistantDiscovery.checked = d.home_assistant_discovery !== false; elements.passwordState.textContent = d.mqtt_password_set ? "A password is saved. Leave empty to keep it." : "No MQTT password is saved."; updateMqttFieldState(); setSaveMessage(""); } catch (e) { setSaveMessage(e.message, "error"); } finally { elements.saveButton.disabled = false; } }
async function saveConfig(event) { event.preventDefault(); if (!elements.settingsForm.reportValidity()) return; const payload = { hostname: elements.hostname.value.trim(), wifi_ssid: elements.wifiSsid.value.trim(), wifi_password: elements.wifiPassword.value, replay_count: Number(elements.replayCount.value), mqtt_enabled: elements.mqttEnabled.checked, mqtt_host: elements.mqttHost.value.trim(), mqtt_port: Number(elements.mqttPort.value || 1883), mqtt_user: elements.mqttUser.value.trim(), mqtt_password: elements.mqttPassword.value, home_assistant_discovery: elements.homeAssistantDiscovery.checked }; elements.saveButton.disabled = true; setSaveMessage("Saving..."); try { const r = await postJson("/api/config", payload); elements.wifiPassword.value = ""; elements.mqttPassword.value = ""; setSaveMessage(r.restart_required ? "Configuration saved. Restarting now; reconnect using the device network IP." : (r.message || "Configuration saved"), "success"); } catch (e) { setSaveMessage(e.message, "error"); } finally { elements.saveButton.disabled = false; } }


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
  document.querySelectorAll(".tab-button").forEach(button => {
    button.addEventListener("click", () => activatePage(button.dataset.page));
  });
  elements.refreshStatusButton.addEventListener("click", loadStatus); elements.refreshAnalyzerButton.addEventListener("click", () => {
    if (analyzerViewMode === "both") loadAnalyzerBothSummary();
    else loadAnalyzer();
  });
  elements.analyzerViewBoth?.addEventListener("click", () => setAnalyzerView("both"));
  elements.analyzerViewRadio1?.addEventListener("click", () => setAnalyzerView("1"));
  elements.analyzerViewRadio2?.addEventListener("click", () => setAnalyzerView("2"));
  elements.refreshSlotsButton.addEventListener("click", loadSlots); elements.refreshRxSlotsButton.addEventListener("click", loadRxSlots);
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
  if (elements.saveRfEnableButton) elements.saveRfEnableButton.addEventListener("click", saveRfEnableSettings);
  if (elements.frequencyScan433Button) elements.frequencyScan433Button.addEventListener("click", () => runFrequencyScan(1));
  if (elements.frequencyScan868Button) elements.frequencyScan868Button.addEventListener("click", () => runFrequencyScan(2));
  if (elements.frequencyScan433TuneButton) elements.frequencyScan433TuneButton.addEventListener("click", () => tuneRadioToDetectedCarrier(1));
  if (elements.frequencyScan868TuneButton) elements.frequencyScan868TuneButton.addEventListener("click", () => tuneRadioToDetectedCarrier(2));
  if (elements.frequencyScan433RestoreButton) elements.frequencyScan433RestoreButton.addEventListener("click", () => restoreRadioDefault(1));
  if (elements.frequencyScan868RestoreButton) elements.frequencyScan868RestoreButton.addEventListener("click", () => restoreRadioDefault(2));
  elements.learnStartButton.addEventListener("click", () => learnAction("/api/radio/learn/start")); elements.learnAcceptButton.addEventListener("click", () => learnAction("/api/radio/learn/accept")); elements.learnSaveButton.addEventListener("click", saveLearnToSlot);
  elements.slotSaveSelect.addEventListener("change", updateSlotSaveSelection); elements.slotSaveCancel.addEventListener("click", closeSlotSaveModal); elements.slotSaveConfirm.addEventListener("click", confirmSlotSave); elements.slotSaveModal.addEventListener("click", event => { if (event.target === elements.slotSaveModal) closeSlotSaveModal(); }); elements.learnTestSendButton.addEventListener("click", () => learnAction("/api/radio/learn/test-send")); elements.learnDiscardButton.addEventListener("click", () => learnAction("/api/radio/learn/discard"));
  elements.otaUploadButton.addEventListener("click", installFirmware); elements.backupRestoreButton.addEventListener("click", restoreBackup);
  renderAnalyzerViewButtons();

  // Restore the page the user was viewing before refresh/reconnect.
  const initialPage = activatePage(pageFromHash(), false);
  if (!window.location.hash || window.location.hash === "#dashboard") {
    window.history.replaceState(null, "", `#${initialPage}`);
  }

  updateMqttFieldState(); loadStatus(); loadRawFrame();
  if (initialPage !== "learn") loadLearnStatus();
  window.setInterval(() => {
    const diagnosticsActive =
      document.getElementById("page-diagnostics")?.classList.contains("active");

    // RF/Protocol diagnostics stay live even while RF Analyzer is enabled.
    // Poll only while the Diagnostics page is visible to avoid background
    // API traffic on normal user-facing pages.
    if (diagnosticsActive) { loadRadioStatus(); loadProtocolTx(); }
  }, 1000);
  window.setInterval(() => {
    const rxSlotsActive =
      document.getElementById("page-rxslots")?.classList.contains("active");
    if (rxSlotsActive) refreshRxSlotLiveStats();
  }, 1000);
  window.setInterval(() => {
    const rawSlotsActive =
      document.getElementById("page-slots")?.classList.contains("active");
    if (rawSlotsActive) refreshRawSlotLiveStats();
  }, 1000);
  window.setInterval(() => {
    const analyzerActive =
      document.getElementById("page-analyzer")?.classList.contains("active");

    if (!analyzerActive || !elements.analyzerDeveloperMode?.checked) return;

    // Step 11: keep firmware-side analysis fully live, but simplify the WebUI.
    // Detailed Radio 1/2 views fetch the complete selected-radio snapshot
    // directly. This removes the previous live -> sequence -> debounce ->
    // full-fetch state machine from the active refresh path.
    if (analyzerViewMode === "both") loadAnalyzerBothSummary();
    else loadAnalyzer();
  }, 500);
  window.setInterval(() => {
    // Analyzer already carries its own RAW preview. Avoid a duplicate RAW API
    // request while the Analyzer page is visible.
    if (!document.getElementById("page-analyzer").classList.contains("active")) {
      loadRawFrame();
    }
  }, 500);
  elements.copyAnalyzerButton?.addEventListener("click", async () => { try { await navigator.clipboard.writeText(elements.analyzerBitstream.textContent); elements.copyAnalyzerButton.textContent = "Copied"; window.setTimeout(() => { elements.copyAnalyzerButton.textContent = "Copy report"; }, 1200); } catch { elements.copyAnalyzerButton.textContent = "Copy failed"; } });
window.setInterval(() => {
  if (document.getElementById("page-learn")?.classList.contains("active")) loadLearnStatus();
}, 1000);
});


window.addEventListener("hashchange", () => {
  activatePage(pageFromHash(), false);
});


// Live header diagnostics: independent from the active page so Core 0/Core 1
// load remains visible while testing RF Learn, Slots, Analyzer, Settings, etc.
window.setInterval(async () => {
  try {
    const d = await requestJson("/api/status");
    renderTuneHeaderStatus(d);
    renderFirmwareVersion(d);
    renderDashboardRadios(d);
    elements.uptime.textContent = formatUptime(d.uptime_seconds);
    elements.ip.textContent = d.ip || "—";
    elements.wifiMode.textContent = d.wifi_mode || "—";
    elements.mqttStatus.textContent = d.mqtt_state || (d.mqtt_enabled ? "Disconnected" : "Disabled");
    setGauge(elements.core0Gauge, elements.core0Load, d.core0_load_percent);
    setGauge(elements.core1Gauge, elements.core1Load, d.core1_load_percent);
    setGauge(elements.psramGauge, elements.psramLoad, d.psram_used_percent);
    setGauge(elements.heapGauge, elements.heapLoad, d.heap_used_percent);
    renderMemoryDiagnostics(d);
  } catch (_) {
    // Keep the last values during a short Wi-Fi/API interruption.
  }
}, 1000);
