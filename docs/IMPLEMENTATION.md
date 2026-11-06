# Engine and implementation

## Audio and MIDI

Eight independent percussion voices run at 44.1 kHz. Each voice combines a morphing oscillator and filtered noise, with separate amplitude envelopes, pitch modulation, stereo positioning, drive and EQ. Per-instance state owns the voices, random generators, caches and effect buffers.

MIDI notes 36–43 and aliases 0–7 trigger the voices. Note Off does not gate these percussion envelopes. Mute/Solo excludes new triggers; a short release handles voices already sounding. Choke groups use deterministic same-frame priority.

The VST2 wrapper is built with SAMPLE_ACCURATE. Events retain their deltaFrames positions; the engine renders variable sub-blocks around their offsets. There is no separate sequencing thread or internal pattern sequencer.

## Shared effects

Two post-level/post-pan send buses feed one shared reverb and one shared delay. Dry and wet signals are summed before the existing output gain and final int16 limiting.

The reverb uses eight damped combs and four allpasses. Size moves interpolated taps; decay determines nominal feedback RT60. The delay uses fractional stereo reads, filtered cross-feedback and a six-second buffer. Cached coefficients and smoothed sends, time, returns and toggles reduce per-sample work and control discontinuities.

SYNC receives host tempo through the wrapper's HAS_LFO_BPM path. It supports 20–400 BPM and falls back to 120 BPM before receiving a valid tempo. Effects skip network processing with no input or remaining tail. Buffers are allocated with the instance; render does not allocate or access files or the network.

Effect tails are transient. OFF fades the input and return while the tail drains; quick re-enable can expose the remainder. Preset/state recall clears the buffers.

## Parameters and Q-Links

The parameter table is append-only and locked by `params.lock`. Canonical voice parameters retain fixed indices; contextual aliases route TONE, NOISE and CHARACTER controls to the selected voice. MIXER maps to eight independent levels. EFFECTS maps to shared controls, including a TIME alias that follows SYNC/FREE and accumulates small discrete nudges.

The skin ships Q-Links.json, Q-Links - 8by1.json and Q-Links - 4by4.json. Each contains Screen Mode and Program Mode sections. The Force's eight physical positions are mapped through slots 13, 9, 5, 1, 14, 10, 6, 2. Unused slots remain -1.

The wrapper retains bounded parameter notifications, logarithmic control normalization and direct parameter lookup. Ordinary writes do not rebuild the whole parameter screen. Skin artwork and filmstrips are shared. The native focus outline and transient azure activity overlays do not change voice selection.

## Native presets and state

The built-in bank exposes 150 VST programs, with full-kit states loaded transactionally. Program enumeration populates the native PRESET menu; XPL browser copies serve a separate purpose.

The PTPROG1 wrapper chunk stores program index and complete edited state. PT1 describes the voices and A/B banks; an optional FX1 suffix stores sends and shared effect settings. Validation rejects incomplete or invalid states before committing them. Re-echoing the current program does not erase restored edits.

Legacy PT1 states load with zero sends and default effects. Effect-enriched states require a binary supporting FX1. No waveform or tail buffer is persisted.

## Sources

- `src/engine.c`: voice engine, MIDI, parameters, notifications and serialization.
- `src/effects.h`: shared effects.
- `scripts/generate.py`: fixed parameter table and generated headers.
- `scripts/build_skin.py`: touch pages and Q-Link variants.
- `scripts/build_factory_programs.py`: full-state factory program table and binaries.
- `third_party/mpc-wrapper/wrapper/vst2_wrap.c`: host integration and chunks.

See THIRD-PARTY.md for upstream provenance and licences.
