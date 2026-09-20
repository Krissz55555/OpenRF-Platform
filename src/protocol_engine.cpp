#include "protocol_engine.h"

#include "protocol_registry.h"
#include "protocols/ev1527_decoder.h"
#include "protocols/pt2262_decoder.h"
#include "protocols/nvkp01_decoder.h"
#include "protocols/ht12e_decoder.h"
#include "protocols/known_protocol_library.h"

#ifndef OPENRF_PROTOCOL_ENGINE_DEBUG
#define OPENRF_PROTOCOL_ENGINE_DEBUG 0
#endif

#if OPENRF_PROTOCOL_ENGINE_DEBUG
#include <Arduino.h>
#endif

namespace {

ProtocolRegistry registry;

}  // namespace

ProtocolRegistry& protocolEngineRegistry() { return registry; }

bool protocolEngineBegin() {
  return knownProtocolLibraryRegisterAll(registry);
}

ProtocolEngineObservation protocolEngineObserve(const RawCapture& capture) {
  ProtocolEngineObservation observation;
  const size_t registeredDecoderCount = registry.count();
  observation.registeredDecoders = static_cast<uint8_t>(registeredDecoderCount);

  for (size_t index = 0; index < registeredDecoderCount; ++index) {
    const ProtocolDecoder* const decoder = registry.at(index);
    if (decoder == nullptr) continue;

    ProtocolMatchResult result;
    if (decoder->protocolId() == ProtocolId::EV1527_PRINCETON) {
      result = ev1527Decoder().decodeDetailed(
          capture, observation.ev1527Diagnostics);
      observation.ev1527DiagnosticsAvailable = true;
    } else if (decoder->protocolId() == ProtocolId::PT2262_TRI_STATE) {
      result = pt2262Decoder().decodeDetailed(
          capture, observation.pt2262Diagnostics);
      observation.pt2262DiagnosticsAvailable = true;
      observation.pt2262Status = result.status;
    } else if (decoder->protocolId() == ProtocolId::NVKP01_KINETIC) {
      result = nvkp01V2Decoder().decodeDetailed(
          capture, observation.nvkp01Diagnostics);
      observation.nvkp01DiagnosticsAvailable = true;
      observation.nvkp01Status = result.status;
    } else if (decoder->protocolId() == ProtocolId::HT12E) {
      result = ht12eDecoder().decodeDetailed(
          capture, observation.ht12eDiagnostics);
      observation.ht12eDiagnosticsAvailable = true;
      observation.ht12eStatus = result.status;
    } else {
      result = decoder->decode(capture);
    }
    if (observation.evaluatedDecoders == 0U ||
        (result.matched() &&
         observation.primaryStatus != ProtocolMatchStatus::MATCH)) {
      observation.primaryProtocol = decoder->protocolId();
      observation.primaryStatus = result.status;
    }
    observation.evaluatedDecoders++;
    if (result.matched()) {
      observation.matchCount++;
      if (observation.candidateCount < kProtocolEngineMaxCandidates) {
        observation.candidates[observation.candidateCount++] = decoder->protocolId();
      }
    } else {
      observation.noMatchCount++;
    }

#if OPENRF_PROTOCOL_ENGINE_DEBUG
    if (decoder->protocolId() == ProtocolId::EV1527_PRINCETON) {
      Serial.print(F("[V2] EV1527 "));
      Serial.println(result.matched() ? F("MATCH") : F("NO_MATCH"));
    } else if (decoder->protocolId() == ProtocolId::PT2262_TRI_STATE) {
      Serial.print(F("[V2] PT2262 "));
      Serial.println(result.matched() ? F("MATCH") : F("NO_MATCH"));
    } else if (decoder->protocolId() == ProtocolId::NVKP01_KINETIC) {
      Serial.print(F("[V2] NVKP01 "));
      Serial.println(result.matched() ? F("MATCH") : F("NO_MATCH"));
    } else if (decoder->protocolId() == ProtocolId::HT12E) {
      Serial.print(F("[V2] HT12E "));
      Serial.println(result.matched() ? F("MATCH") : F("NO_MATCH"));
    }
#endif
  }

  observation.decision = protocolEngineDecisionFromMatchCount(observation.matchCount);
  observation.selectedProtocol =
      observation.decision == ProtocolEngineDecisionState::KNOWN
          ? (observation.candidateCount > 0U ? observation.candidates[0]
                                             : observation.primaryProtocol)
          : ProtocolId::UNKNOWN;

  // Step 29.8: materialize one compact normalized event only when the engine
  // has exactly one KNOWN candidate and that decoder produced a complete code.
  // This remains local shadow metadata; nothing is published or queued here.
  if (observation.decision == ProtocolEngineDecisionState::KNOWN) {
    NormalizedRfEvent event;
    event.protocol = observation.selectedProtocol;
    event.radioId = capture.radioId();
    event.rawPulseCount = capture.pulseCount();
    event.rawDurationUs = capture.durationUs();
    event.capturedAtMs = capture.capturedAtMs();
    event.frequencyMHz = capture.frequencyMHz();
    event.rssiDbm = capture.rssiDbm();

    if (event.protocol == ProtocolId::EV1527_PRINCETON &&
        observation.ev1527DiagnosticsAvailable &&
        observation.ev1527Diagnostics.codeAvailable) {
      event.available = true;
      event.code = observation.ev1527Diagnostics.decodedCode;
      event.symbolCount = ev1527DecoderLimits().expectedBitsPerFrame;
      event.repeats = observation.ev1527Diagnostics.matchingRepeatCount;
    } else if (event.protocol == ProtocolId::PT2262_TRI_STATE &&
               observation.pt2262DiagnosticsAvailable &&
               observation.pt2262Diagnostics.codeAvailable) {
      event.available = true;
      event.code = observation.pt2262Diagnostics.decodedCode;
      event.symbolCount = observation.pt2262Diagnostics.decodedTritCount;
      event.repeats = observation.pt2262Diagnostics.matchingRepeatCount;
    } else if (event.protocol == ProtocolId::NVKP01_KINETIC &&
               observation.nvkp01DiagnosticsAvailable &&
               observation.nvkp01Diagnostics.codeAvailable) {
      event.available = true;
      event.code = observation.nvkp01Diagnostics.normalizedCode;
      event.symbolCount = 1U;
      event.repeats = observation.nvkp01Diagnostics.repeatCount;
    } else if (event.protocol == ProtocolId::HT12E &&
               observation.ht12eDiagnosticsAvailable &&
               observation.ht12eDiagnostics.codeAvailable) {
      event.available = true;
      event.code = observation.ht12eDiagnostics.decodedWord;
      event.symbolCount = ht12eDecoderLimits().expectedBits;
      event.repeats = observation.ht12eDiagnostics.repeatCount;
    }

    if (event.available) observation.normalizedEvent = event;
  }

  // Step 29.10: only a real KNOWN normalized event enters the burst-collapse
  // gate. UNKNOWN/AMBIGUOUS captures (including noise) do not update dedup
  // state and therefore cannot break or extend a known-event burst.
  observation.dedup = normalizedEventDedupObserve(observation.normalizedEvent);

  // Step 29.11: model the exact post-dedup action boundary without calling
  // any action target. Only an EMIT logical event can become a dry-run
  // actionable candidate. Collapsed repeats and UNKNOWN/AMBIGUOUS captures
  // remain non-actionable.
  observation.actionableDryRun =
      v2ActionableDryRunObserve(observation.normalizedEvent, observation.dedup);

  return observation;
}

const char* protocolEngineDecisionStateName(ProtocolEngineDecisionState state) {
  switch (state) {
    case ProtocolEngineDecisionState::KNOWN:
      return "KNOWN";
    case ProtocolEngineDecisionState::AMBIGUOUS:
      return "AMBIGUOUS";
    default:
      return "UNKNOWN";
  }
}
