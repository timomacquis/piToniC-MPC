# πToniC — architecture actuelle

Base : **1.0.2**. Les [spécifications](SPECIFICATIONS.md) décrivent le périmètre ; les [notes de release](RELEASE-NOTES-1.0.2.md) précisent ce qui a été qualifié.

## Synthèse et MIDI

Huit voix dans une instance VST2. Le contrat du moteur est 44 100 Hz, stéréo signé 16 bits ; le wrapper convertit les tranches de rendu de 1 à 128 frames vers la sortie float de l’hôte.

Oscillateur sinus/triangle/dent-de-scie/carré avec morphing, bruit filtré, enveloppes d’amplitude et de hauteur, sensibilités à la vélocité, drive et EQ par voix. Les paramètres dérivés sont mis en cache ; voix silencieuses et chemins inutilisés évitent leurs calculs. Les changements de niveau, retriggers et releases utilisent de courtes transitions.

Les notes 0–7 et 36–43 adressent A01–A08. Les Note Off ne raccourcissent pas les enveloppes de percussion. Le MIDI ne déplace pas la sélection d’édition. Le Step Sequencer natif a été validé sur la Force ; il ne s’agit pas d’un séquenceur ajouté au moteur.

## SAMPLE et affichage

Le worker de chargement possède les accès fichiers, allocations et libérations des WAV. Le rendu n’effectue pas ces opérations. La substitution du sample remplace la source tonale ; bruit, enveloppes et traitements restent disponibles.

Les extrema min/max sont calculés au chargement, puis mis en cache : 288 colonnes par canal. START/TUNE ne recalculent ni ne renvoient l’onde. Le cadrage de fin silencieuse ne change ni le PCM ni le pourcentage START enregistré. Voir [les règles précises du LCD](SAMPLE-LCD-1.0.2.md).

## Effets partagés


Eight existing voices feed two stereo buses after level, pan, retrigger crossfade and mute/solo release. Each bus owns one shared effect instance, never eight per-voice effects. The dry mix remains unchanged. Returns are summed before the existing output gain and final int16 clipping.

- Reverb: eight damped feedback combs, four allpasses; independent left/right networks. SIZE changes interpolated delay taps smoothly, DECAY sets nominal feedback RT60, DAMPING controls the loop low-pass. Diffusion and filtering affect perceived decay.
- Delay: stereo fractional ring reads, cross-channel filtered feedback. SYNC uses native host tempo via the existing HAS_LFO_BPM wrapper hook; FREE uses 20–1500 ms. Divisions: 1/16, 1/8, dotted 1/8, 1/4, 1/2. A six-second buffer covers 20–400 BPM; initial fallback is 120 BPM. No new clock thread or MIDI port.
- Tone is a cached low-pass coefficient. Feedback is limited to 90%. Sends, returns and effect toggles ramp. Time changes use a smoothed read position and can pitch-shift the existing repeats.
- Networks stop processing when no input/tail is present. Silence and feedback paths suppress tiny denormal values. Rings are allocated once with the engine instance (about 2.2 MiB for the effect structure), even when unused; no per-sample allocation or file I/O. This memory allocation is not a device CPU measurement.
- OFF fades the input and return; the internal tail drains. Rapid re-enable may reveal the remaining tail. Mute/Solo excludes new triggers and releases the voice; already accumulated effect tails continue. A discrete preset/state load clears transient effect buffers.

The architecture is adapted from the preserved Weird Dreams source by Vincent Fillion (based on WeirdDrums); the new bounded DSP is in src/effects.h. It does not compile the upstream compressor, master EQ or full application.


## Paramètres, presets et Q-Links

Les paramètres sont ajoutés en fin de table, jamais réordonnés. Les contrôles visibles TONE et les Q-Links physiques utilisent les mêmes alias contextuels `tone_q` : deux chemins cohérents vers les paramètres canoniques de la voix sélectionnée.

Le menu PRESET énumère des programmes VST : déposer des XPL dans un dossier de sauvegarde ne suffit pas. La banque native contient 150 programmes Patternarium BEST. Le wrapper garde l’index du programme et l’état édité ; il ne recharge pas la recette d’usine à la réouverture au détriment des modifications.

A/B compare les timbres d’une voix ; sends et effets partagés restent hors de cette comparaison. Les états PT1 historiques sont complétés par des extensions validées avant application. Les buffers de réverbération/delay, l’activité et les caches graphiques ne sont pas sauvegardés comme sons en cours.

Notifications natives bornées, ressources graphiques partagées, absence de reconstruction globale pour les simples réglages : conserver ces propriétés lors de toute extension.

[Guide générique : conventions exactes des Q-Links et des presets](../../Generic-Plugin-Development.md).

## Compilation et preuves

[Compilation](BUILD.md) · [Installation](INSTALLATION.md). Les tests hôte et moteurs ne prouvent pas à eux seuls le comportement tactile ou musical. Les rapports datés de `Validation/` gardent leur périmètre original.

[Implémentation initiale](IMPLEMENTATION.md) et [première extension d’effets](EFFECTS-IMPLEMENTATION.md) : historique complet, avec les anciens nombres de paramètres et attentes de validation.
