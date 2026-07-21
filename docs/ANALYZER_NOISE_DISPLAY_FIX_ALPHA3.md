# v0.7.2-alpha3 — Analyzer noise display fix

- Rejected receiver edge bursts remain counted in radio diagnostics.
- During normal monitoring, rejected noise no longer advances the Analyzer sequence or replaces the last meaningful signal.
- During RF Learn, rejected captures remain visible for protocol-development diagnostics.
- RAW Learn and RAW Replay are unchanged.

This separates physical edge-burst candidates from logical RF frames.
