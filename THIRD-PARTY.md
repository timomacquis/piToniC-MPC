# Third-party material

- **WeirdDrums**, Daniele Filaretti, MIT. Reference snapshot: `6b2cd2525913109d05ab08bcaf559189b8ea529d`. Original licence preserved in `third_party/weirddrums/LICENSE`.
- **Weird Dreams for Move**, Vincent Fillion, based on WeirdDrums. Reference snapshot: `eabd121d1ffd5d98c077633edeb18df90f85124b`. Upstream README explicitly states MIT; no standalone LICENSE file was present in the inspected tree. The preserved README and DSP source carry the attribution. πToniC adapts the oscillator/noise/envelope architecture into a separate per-instance MPC engine and adds its own parameter/state handling, morph, envelope modes, stereo noise, EQ and choking. The effects extension adapts the damped-comb/allpass reverb and filtered cross-feedback delay architecture into `src/effects.h`, with cached coefficients, smoothed controls, per-voice stereo sends, native-tempo synchronization and instance-local buffers. The full upstream master bus is not compiled.
- **MPC VST wrapper**, sd88me, MIT. Base commit `97a2effca4114dd00f738b018e2f0d1d3ccb2d7f`, with a local πClideaN-derived snapshot and πToniC changes. See `third_party/mpc-wrapper/VENDORED.json` for hashes and inherited provenance. The host ABI is handwritten; no Steinberg SDK is shipped.
- **Open MPC release tools**, MIT. Pinned copied subset and provenance in `third_party/open-mpc-release/UPSTREAM.json`. These generate the local install/uninstall package.
- **Titillium Web**, SIL Open Font License. See `reference/fonts/OFL.txt`.

Sonic Charge Microtonic is a musical reference only. No Microtonic binary, preset collection or proprietary DSP is included in the runtime. TR-MPC was inspected to understand host routing; no GPL TR-MPC source is incorporated in πToniC.
