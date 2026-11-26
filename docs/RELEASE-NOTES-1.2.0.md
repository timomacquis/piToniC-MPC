# πToniC 1.2.0

- **New WAVETABLE source**: SOURCE now offers **SYNTH · WAVETABLE · SAMPLE**. Eight tables written from formulas — PWM, HARMONICS, FORMANT, SYNC, FOLD, CRUSH, FM and NOISE — each chaining 16 waveforms, precomputed without aliasing for every pitch range (2.4 MB shared by all instances; per-voice cost close to the SYNTH oscillator).
- **Per-hit scan, independent of pitch**: **POS** (start), **END**, **SCAN TIME** (1–10,000 ms) and **SCAN** (ONE, LOOP, PING-PONG). The scan restarts on every hit. Settings belong to each voice and each A / B bank, and can be automated or locked per step. RAND VOICE also draws them; ALTER varies END and SCAN TIME.
- **TONE page**: a 4 × 2 grid for SOURCE and TABLE / SCAN (opened like PITCH MODE, value shown as text), a table display that follows POS (drag it to set POS), then FREQUENCY, POS, END, SCAN TIME.
- **Q-Links in WAVETABLE**: Frequency, Pos, End, Scan Time, Amp Attack, Amp Decay, Pitch Amount, Pitch Time / Rate, then Pitch Velocity (second row). SYNTH and SAMPLE keep their 1.1.2 order.
- **Compatibility**: SYNTH and SAMPLE sound exactly as before; the 150 built-in kits, existing projects and `.mtpreset` import are unchanged. SAMPLE stays the top value of SOURCE, so automation or locks recorded on SAMPLE remain SAMPLE. Projects that use WAVETABLE cannot be opened by 1.1.2.

UID and published parameter indices are unchanged (50 parameters added at the end). Gen2 remains untested on hardware.
