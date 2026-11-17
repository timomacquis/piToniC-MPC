# πToniC 1.0.4

- **Installation follows Akai's SDK rule**: each release is a plugin folder to copy into `VST2 Plugins` (Gen1 ARM32 and Gen2 ARM64 ZIPs). No installer script, no `Synths` folder, no MPC.settings registration. Remove an older catalogue installation with its own `uninstall.sh` first.
- The 150 Patternarium BEST kits remain native programs inside the binary (PRESET arrow).
- Project recall no longer gives up silently when the sample loader is busy; sample browser clicks and loads are no longer dropped.
- WAV files are read in one pass: faster loading of long samples.
- Colour charter: blue outline on PITCH MODE, FILTER MODE, NOISE ENVELOPE, delay SYNC / FREE and TIME division; amber once CHOKE GROUP or STEREO NOISE leaves OFF; gold sample waveform; azure activity unchanged.
- New test: project state size against the 8 KB host chunk (fits with eight 250-character sample paths).

UID, parameters, presets and project format are unchanged. Gen2 remains untested on hardware.
