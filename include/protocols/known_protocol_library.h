#pragma once

#include <stddef.h>

#include "protocol_decoder_v2.h"

class ProtocolDecoder;
class ProtocolRegistry;
class ProtocolTxEncoder;

// Step 30/34 modular Known Protocol Library. Decoder and optional TX encoder
// objects are statically lived and never owned by the library.
size_t knownProtocolLibraryCount();
const ProtocolDecoder* knownProtocolLibraryAt(size_t index);
bool knownProtocolLibraryRegisterAll(ProtocolRegistry& registry);

// Step 34 optional TX capability discovery. RX-only modules simply return
// nullptr/false here and require no dummy TX implementation.
const ProtocolTxEncoder* knownProtocolLibraryTxEncoder(ProtocolId protocol);
bool knownProtocolLibrarySupportsTx(ProtocolId protocol);
