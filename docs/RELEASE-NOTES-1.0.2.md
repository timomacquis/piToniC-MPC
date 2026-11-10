# πToniC 1.0.2

A Microtonic-inspired drum machine.

πToniC features 8 independent drum voices, each combining filtered noise with a morphing oscillator or WAV sample playback.

Its sound engine includes pitch modulation, amplitude envelopes, drive, EQ and individual voice controls.

Shared stereo reverb and tempo-synced delay complete the engine.

- Fix TONE control bindings so visible controls and contextual Q-Links use the same TUNE/START aliases in SAMPLE and FREQUENCY/WAVE MORPH aliases in SYNTH.
- Add a black LCD sample overview: one waveform for mono, separate left/right traces for stereo and a dashed START marker.
- Frame the audible sample tail automatically with a small margin, preserving leading silence. This only changes the display: PCM, playback duration and the stored START percentage remain unchanged.
- Cache the display on the loading worker; turning START or TUNE does not rebuild or resend the waveform.
- Reduce LOAD height and balance its vertical spacing.
- Refresh English/French manuals with current Force screenshots and place SAMPLE inside TONE.

All 418 parameters from 1.0.1 retain their indices. Passive display parameters are appended. The 150 native Patternarium BEST kits and original sample paths are preserved. Keep older project copies: older binaries cannot read sample-enriched states.

Gen1 ARMv7 / Force, MPC OS 3.x. Based on the candidate installed on the author's Force. TUNE/START routing and marker movement were confirmed on candidate.1; the revised framing and LOAD spacing were installed as candidate.2. No new exact-release install/uninstall qualification or Gen2 hardware claim is made.
