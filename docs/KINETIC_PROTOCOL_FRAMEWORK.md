# Kinetic Protocol Framework scaffold

This development scaffold introduces a protocol-independent event model and the
first decoder skeleton (`NVKP01 Kinetic`). It is intentionally not connected to
the live RX path yet.

## Added files

```text
src/protocols/protocol.h
src/protocols/kinetic/kinetic_event.h
src/protocols/kinetic/kinetic_decoder.h
src/protocols/kinetic/nvkp01/decoder.h
src/protocols/kinetic/nvkp01/decoder.cpp
src/protocols/kinetic/nvkp01/README.md
src/protocols/kinetic/nvkp01/notes.md
src/protocols/kinetic/nvkp01/samples.txt
```

## Safety boundary

- No existing decoder was replaced.
- No call was added to `radio.cpp`, `rxslots.cpp`, MQTT or Home Assistant code.
- The NVKP01 stub always returns `false`.
- Existing runtime behaviour is therefore unchanged.
