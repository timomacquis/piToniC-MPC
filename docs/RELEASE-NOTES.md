# πToniC 1.1.2

- **MicroTonic kit import**: new **IMPORT KIT** button at the bottom left of the MIXER page. It opens the file browser on `.mtpreset` files (MicroTonicPresetV3). The eight sounds replace banks A and B of the eight voices; all voices switch to SYNTH, bank A, no mute or solo; REVERB / DELAY sends are set to 0. Effect settings and loaded samples are kept. Patterns, tempo and swing are ignored: the MPC sequences the kit. A status line reports the result or why a file was refused (a refused file changes nothing).
- **Ranges aligned with MicroTonic** wherever the engine can follow: AMP DECAY and NOISE DECAY from 1 ms (was 10 ms), CUTOFF and EQ FREQUENCY up to 20 kHz (was 18 kHz), EQ GAIN ±40 dB (was ±24 dB). Existing projects and presets keep their sound; an automation already recorded on one of these six knobs is shifted because their travel changed. RANDOM keeps its previous ranges.
- **Import rules**: values pass unchanged where ranges match. RESONANCE is the only range the filter cannot reach (MicroTonic Q up to 10 000): the Q is placed at the same ratio of the πToniC range 0.1–20. Infinite pitch decay becomes 20 s; levels above 0 dB are lowered together, keeping the balance between voices. Full rules: [MTPRESET-IMPORT-1.1.2.md](MTPRESET-IMPORT-1.1.2.md).
- **The 150 Patternarium BEST kits are reconverted** with these rules: same kits, order and names; a built-in kit now gives exactly the values of importing its `.mtpreset`. Noise resonance is much lower than in 1.1.0 on most kits.

UID and published parameter indices are unchanged (three parameters added at the end). Gen2 remains untested on hardware.

---

# πToniC 1.0.4

- **Installation follows Akai's SDK rule**: each release is a plugin folder to copy into `VST2 Plugins` (Gen1 ARM32 and Gen2 ARM64 ZIPs). No installer script, no `Synths` folder, no MPC.settings registration. Remove an older catalogue installation with its own `uninstall.sh` first.
- The 150 Patternarium BEST kits remain native programs inside the binary (PRESET arrow).
- Project recall no longer gives up silently when the sample loader is busy; sample browser clicks and loads are no longer dropped.
- WAV files are read in one pass: faster loading of long samples.
- Colour charter: blue outline on PITCH MODE, FILTER MODE, NOISE ENVELOPE, delay SYNC / FREE and TIME division; amber once CHOKE GROUP or STEREO NOISE leaves OFF; gold sample waveform; azure activity unchanged.
- New test: project state size against the 8 KB host chunk (fits with eight 250-character sample paths).

UID, parameters, presets and project format are unchanged. Gen2 remains untested on hardware.

---

# πToniC 1.0.3

A Microtonic-inspired drum machine.

πToniC features 8 independent drum voices, each combining filtered noise with a morphing oscillator or WAV sample playback.

Its sound engine includes pitch modulation, amplitude envelopes, drive, EQ and individual voice controls.

Shared stereo reverb and tempo-synced delay complete the engine.

- Replace full-page selectors with compact panels sized to their contents.
- Keep the current screen visible under a dim overlay and highlight the opening selector in yellow.
- Use 36 px buttons within panels, preserving the existing main-screen control sizes.
- Apply the design to voice option selectors and delay divisions.
- Make the sample browser compact, retaining eight entries, navigation and the existing larger text sizes.

Sound synthesis, sample playback, contextual Q-Links, parameter indices, saved states and the 150 Patternarium BEST kits are preserved.

Gen1 ARMv7 / Force. The skin was installed on the author's Force. Offline checks compare original control bindings, geometry, native selector encoding, parameter identity and Q-Links. No new exhaustive touch/audio or exact-package install/uninstall qualification is claimed; no Gen2 package is included.
