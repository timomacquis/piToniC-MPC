# πToniC — User Manual

A Microtonic-inspired drum machine.

πToniC features 8 independent drum voices, each combining filtered noise with a morphing oscillator or WAV sample playback.

Its sound engine includes pitch modulation, amplitude envelopes, drive, EQ and individual voice controls.

Shared stereo reverb and tempo-synced delay complete the engine.

Explore **150 converted Patternarium BEST kit presets**, then make them your own.

## 1. Play

**A01–A08** select a voice for editing. MIDI notes **36–43** trigger the eight voices without changing your selection. Notes 0–7 are also accepted as drum-path aliases.

Use the Force's own Step Sequencer. For notes 36–43 on A01–A08:

**SHIFT + STEP SEQUENCER → Scale: Chromatic → Snap to Scale: On.**

Check the starting register. **TRIGGER** previews the selected voice. Envelopes set the sound's duration; releasing the pad does not cut the hit.

The selected voice is red. A thin white outline follows the focused control for the data wheel. An azure pulse marks an accepted trigger; muted voices and voices excluded by Solo do not flash.

![πToniC — A02 activity](images/activity-force.png)

A02 is playing: its button pulses azure while A03 remains selected for editing.

## 2. TONE

Choose **SYNTH** to shape a sound, or **SAMPLE** to start with a WAV. Each voice has its own choice.

### SYNTH — shape a hit

![πToniC — TONE](images/tone-force-1.0.2.png)

- **FREQUENCY** tunes the oscillator.
- **WAVE MORPH** moves through sine, triangle, saw and square.
- **AMP ATTACK / DECAY** shape the amplitude envelope.
- **PITCH MODE** selects a decay envelope, sine modulation or random modulation.
- **PITCH AMOUNT** sets the pitch excursion in semitones.
- **PITCH TIME / RATE** sets milliseconds in DECAY mode and Hz in SINE / RANDOM. The effective sine rate is capped at 2,000 Hz.
- **PITCH VELOCITY** makes pitch modulation respond to playing velocity.

### SAMPLE — start with a sound

![πToniC — SAMPLE](images/sample-force-1.0.2.png)

A recorded kick? A texture? A found sound?

Choose **SAMPLE**, press **LOAD**, then tap your WAV file.

**USER / SD** and **DRIVES** choose the storage location. **UP** goes up a folder; **PREVIOUS / NEXT** turn the pages. **CANCEL** closes the browser.

Mono or stereo WAV, **up to 10 seconds**: PCM 16/24/32-bit or float32, 8–192 kHz. The browser shows up to 256 entries per folder.

**TUNE** shifts pitch by up to ±48 semitones. Zero plays at the original speed. Higher pitches also make the sound shorter.

**START** chooses the starting point as a percentage of the whole file.

Each note retriggers the sample: one-shot playback, without looping or time-stretch.

The **AMP** envelope, pitch modulation, noise and effects remain available. Try a short sample with a little noise: a new percussion sound takes shape.

### Read the LCD

One waveform for mono. Two for stereo.

The display frames the sound with a small margin after its tail. Leading silence stays visible; the file and playback stay unchanged.

The **dashed line** marks START. If the starting point lies beyond the displayed region, the line stays at the right edge.

The waveform is prepared at loading and cached. Loading errors appear in place of the filename.

### Keep your sounds together

**A / B** keeps two TUNE/START settings for the same file. **ALTER** and **RAND VOICE** keep the sample while varying its settings.

The first two Q-Links become **TUNE and START**. Switch back to SYNTH and they return to **FREQUENCY and WAVE MORPH**.

**Presets and projects store the sample path, not the audio itself.** Keep files in their original locations. Automatic collection beside the preset is not available yet.

## 3. NOISE

![πToniC — NOISE](images/noise-force-1.0.2.png)

**NOISE MIX** balances oscillator and noise.

**FILTER MODE** selects LP, BP or HP. Shape it with **CUTOFF** and **RESONANCE**.

**NOISE ATTACK / DECAY** set the noise envelope. Choose **EXP**, **LINEAR** or **CLAP** with NOISE ENVELOPE.

**STEREO NOISE** uses independent noise sources on the two sides. OFF uses a common source.

## 4. CHARACTER

![πToniC — CHARACTER](images/character-force-1.0.2.png)

Want a little bite? Turn up **DRIVE**.

Shape the colour with **EQ FREQUENCY / GAIN**.

**LEVEL** sets its volume. **OSC VELOCITY / NOISE VELOCITY** control the two velocity responses. **PAN** places the voice in stereo.

Voices in the same **CHOKE GROUP** cut one another off. Choose OFF to let them ring independently.

## 5. EFFECTS

![πToniC — EFFECTS](images/effects-force-1.0.2.png)

Space on the left: **REVERB**.

Echoes on the right: **DELAY**.

All eight voices share these effects. Feed them from the MIXER sends; the dry sound stays present. The effects run in parallel.

### Reverb

- **SIZE** changes the reverb network's size.
- **DECAY** sets a nominal decay of 0.2–8 seconds. Filtering and diffusion affect the perceived tail.
- **DAMPING** darkens the tail.
- **RETURN** sets the overall reverb level.

### Delay

**SYNC** follows MPC's tempo. Touch TIME to choose **1/16, 1/8, 1/8 D, 1/4 or 1/2**. D means dotted.

Switch to **FREE** for **20–1,500 ms**.

**FEEDBACK** sets the repeats, up to 90%. **TONE** filters their high frequencies. **RETURN** sets the overall delay level.

The stereo delay crosses its feedback between left and right. Changing TIME moves the read position smoothly and can bend the pitch of existing repeats.

SYNC uses valid host tempo between 20 and 400 BPM; it falls back to 120 BPM before receiving tempo. The buffer covers all offered divisions within that range.

**ON / OFF** fades the effect input and return. The internal tail drains while OFF; quickly re-enabling can reveal the remainder.

## 6. MIXER

![πToniC — MIXER](images/mixer-force-1.0.2.png)

Each voice has **LEVEL, PAN, REVERB, DELAY, M and S**.

**REVERB / DELAY** set its two sends, after level and pan. Start with one send, then adjust the effect's RETURN.

**M** silences the voice.

**S** plays only soloed voices. Multiple solos are allowed; Mute takes precedence.

Active toggles are amber. Existing effect tails may continue after muting a voice.

All factory sends start at zero: raise a send to hear an effect. Networks with no input or tail skip their audio processing.

## 7. Explore

**A / B** compares two sound states for the selected voice.

**ALTER** makes a small variation.

**RAND VOICE** creates a new timbre.

These preserve LEVEL, PAN, MUTE, SOLO, CHOKE, sends and shared effects. Sends are mix settings shared by both A/B sound states.

## 8. Q-Links

Left to right on the Force's eight physical encoders:

| Page | Controls |
| --- | --- |
| TONE · SYNTH | Frequency · Wave Morph · Amp Attack · Amp Decay · Pitch Amount · Pitch Time / Rate · Pitch Velocity · unused |
| TONE · SAMPLE | Tune · Start · Amp Attack · Amp Decay · Pitch Amount · Pitch Time / Rate · Pitch Velocity · unused |
| NOISE | Noise Mix · Cutoff · Resonance · Noise Attack · Noise Decay · unused · unused · unused |
| CHARACTER | Drive · EQ Frequency · EQ Gain · Level · Osc Velocity · Noise Velocity · Pan · unused |
| EFFECTS | Reverb Size · Decay · Damping · Return · Delay Time · Feedback · Tone · Return |
| MIXER | Level A01 through A08 |

TONE, NOISE and CHARACTER follow the selected voice. EFFECTS controls the shared processors; its TIME encoder follows SYNC / FREE. MIXER always controls all eight levels. Use touch and the data wheel for sends.

## 9. Presets and projects

The native **PRESET** menu lists 150 converted Patternarium BEST kits. Selecting a kit recalls all eight voices and their A/B sound states. The first program loads when you insert the instrument.

Use MPC's native preset save function to keep your own kits, including sends and effects. A project stores the selected program and your edits. Audio tails are transient and are cleared on state recall.

Legacy πToniC PT1 states load with zero sends and default effects. Older binaries cannot load new effect-enriched states.

The kits are adapted parameter recipes for πToniC's own engine. They do not reproduce Microtonic's sound exactly. Source credits accompany the bank.
