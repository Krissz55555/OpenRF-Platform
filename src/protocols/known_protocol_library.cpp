#include "protocols/known_protocol_library.h"

#include "protocol_registry.h"
#include "protocol_tx.h"
#include "protocols/ev1527_decoder.h"
#include "protocols/ev1527_tx.h"
#include "protocols/pt2262_decoder.h"
#include "protocols/pt2262_tx.h"
#include "protocols/nvkp01_decoder.h"
#include "protocols/ht12e_decoder.h"
#include "protocols/ht12e_tx.h"

namespace {
struct KnownProtocolModule final {
  const ProtocolDecoder* decoder;
  const ProtocolTxEncoder* tx;
};

const KnownProtocolModule kModules[] = {
    {&ev1527Decoder(), &ev1527TxEncoder()},
    {&pt2262Decoder(), &pt2262TxEncoder()},
    {&nvkp01V2Decoder(), nullptr},
    {&ht12eDecoder(), &ht12eTxEncoder()},
};
constexpr size_t kModuleCount = sizeof(kModules) / sizeof(kModules[0]);
}  // namespace

size_t knownProtocolLibraryCount() { return kModuleCount; }

const ProtocolDecoder* knownProtocolLibraryAt(size_t index) {
  return index < kModuleCount ? kModules[index].decoder : nullptr;
}

bool knownProtocolLibraryRegisterAll(ProtocolRegistry& registry) {
  if (registry.count() == 0U) {
    for (size_t i = 0; i < kModuleCount; ++i) {
      if (!registry.add(kModules[i].decoder)) return false;
    }
    return true;
  }
  if (registry.count() != kModuleCount) return false;
  for (size_t i = 0; i < kModuleCount; ++i) {
    if (registry.at(i) != kModules[i].decoder) return false;
  }
  return true;
}

const ProtocolTxEncoder* knownProtocolLibraryTxEncoder(ProtocolId protocol) {
  for (size_t i = 0; i < kModuleCount; ++i) {
    if (kModules[i].decoder != nullptr &&
        kModules[i].decoder->protocolId() == protocol) {
      return kModules[i].tx;
    }
  }
  return nullptr;
}

bool knownProtocolLibrarySupportsTx(ProtocolId protocol) {
  return knownProtocolLibraryTxEncoder(protocol) != nullptr;
}
