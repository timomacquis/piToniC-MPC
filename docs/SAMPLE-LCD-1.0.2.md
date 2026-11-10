# πToniC 1.0.2 — SAMPLE LCD

Gen1 maintenance release based on the installed candidate. See release notes for validation scope.

The SAMPLE page shows a fixed black LCD overview: 288 signed min/max columns per channel, one centered trace for mono or separate stereo traces. A dashed START marker follows the existing start setting. There is no zoom or playback animation.

Waveform extrema are calculated on the sample-loading worker and cached. Moving START or TUNE does not rebuild or resend the waveform. Display updates retain the existing bounded notification budget.

Visible TONE controls now use the same contextual tone_q aliases as the physical Q-Links: TUNE/START in SAMPLE, FREQUENCY/WAVE MORPH in SYNTH. Canonical automation also refreshes these aliases. The reported wrong hardware routing still requires confirmation on Force; host tests cannot establish native controller behavior.

The original 418 parameter indices, sample paths, A/B state, presets and audio processing are preserved. New passive display parameters are appended. No audio samples are distributed.

Candidate 2: display-only trailing-silence framing at -60 dB relative to the peak, with a 5% (minimum 2 ms) margin. Leading silence is preserved. Silence-only files keep the full overview. The original PCM and START percent remain unchanged; the dashed marker maps to the displayed duration and stays at the right edge when START is beyond the visible area. LOAD is reduced to 30 pixels high with equal 15-pixel vertical margins. Candidate 1 TUNE/START and marker movement were confirmed by Greg; candidate 2 display framing is pending hardware verification.
