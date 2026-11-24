# πToniC 1.1.2 — moteur et règles d'import `.mtpreset` (référence pour FM-1)

24 novembre 2026. Ce document décrit tout ce qui a changé dans le moteur de πToniC entre 1.1.0 et 1.1.2, et les règles complètes de conversion d'un fichier `MicroTonicPresetV3` en kit πToniC, pour les reporter dans f-ToniC (FM-1).

Implémentations de référence, identiques à la valeur près (vérifié sur 150 kits et 4 fichiers de test) :
- C : `src/mtpreset.h` (lecteur embarqué dans le plugin) ;
- Python : `convert()` dans `scripts/convert_microtonic_private.py` (banque embarquée).

## 1. Principe

1. Là où le moteur peut reproduire la plage de Microtonic, la plage de πToniC est la même et la valeur passe **telle quelle**.
2. Là où le moteur ne peut pas suivre, la valeur garde **sa position relative** : `min πToniC + (x − min MT) / (max MT − min MT) × (max πToniC − min πToniC)`. Seul cas en 1.1.2 : RESONANCE.
3. Une valeur infinie (ModRate `inf`) ou hors de la plage de πToniC après conversion (ex. un decay sous 1 ms, un Q hors 0,1–10 000) est **ramenée à la limite** et comptée comme « value limited ». Les fichiers générés dépassent parfois la plage du manuel (Noise Decay 1 ms pour un minimum documenté de 10 ms) : πToniC accepte ces valeurs dès que son moteur les supporte.

Plages Microtonic : manuel utilisateur v3.3.4 (`π Instruments research/piToniC/Microtonic User Guide.pdf`).

## 2. Changements du moteur

### 1.1.1 — import de kit

- Lecteur `.mtpreset` borné, sans évaluation : 1 Mio, 20 000 éléments, profondeur 16, 10 000 valeurs, 128 champs par groupe. Le kit n'est écrit que si **tout** le fichier est valide ; sinon rien ne change et un message dit pourquoi (`Sound 5: invalid DistAmt`).
- Lecture et conversion sur le thread du worker de samples (I/O, allocation) ; le kit est appliqué au début du `render` par un échange atomique `pending` / `retired`, sans verrou ni allocation dans le thread audio.
- Application : les 8 sons remplacent les banques **A et B** ; toutes les voix passent en SYNTH, banque A, sans mute ni solo ; sends REVERB / DELAY à 0. Réglages des effets, samples chargés et TUNE / START conservés.
- Ignorés (syntaxe vérifiée seulement) : Patterns, Tempo, Swing, FillRate, MastVol, Mutes, Pattern courant.
- Paramètres ajoutés en fin de liste (1 017 → 1 020, aucun indice déplacé) : `kit_import`, `kit_browse`, `kit_status`.

### 1.1.2 — plages alignées sur Microtonic

| Champ πToniC | Avant | 1.1.2 | Microtonic |
|---|---|---|---|
| `decay` AMP DECAY | 10–10 000 ms | **1**–10 000 ms | Osc Decay |
| `ndcy` NOISE DECAY | 10–10 000 ms | **1**–10 000 ms | 10 ms–10 s (fichiers générés jusqu'à 1 ms) |
| `cutoff` CUTOFF | 20–18 000 Hz | 20–**20 000** Hz | 20 Hz–20 kHz |
| `eqfreq` EQ FREQUENCY | 20–18 000 Hz | 20–**20 000** Hz | 20 Hz–20 kHz |
| `eqgain` EQ GAIN | ±24 dB | **±40** dB | ±40 dB |
| `res` RESONANCE | 0,1–20 | 0,1–20 (inchangé) | 0,1–10 000 |

Détails moteur :
- Filtres biquad (bruit et EQ) : la fréquence était bornée en interne à `0,45 × SR` = 19 845 Hz à 44,1 kHz ; elle l'est maintenant à 20 000 Hz (Nyquist 22 050 Hz). Aucune valeur possible avant 1.1.2 n'est concernée : le rendu des presets existants est identique (régression CLASSIC identique au bit près).
- Enveloppes : `env_start` gérait déjà les durées courtes (au moins 1 échantillon) ; 1 ms = 44 échantillons. Clics audibles possibles à 1 ms, comme dans Microtonic.
- EQ en cloche (RBJ, Q 0,707) : ±40 dB sans changement de code.
- RESONANCE reste bornée à 0,1–20 : au-delà, le biquad float devient instable et, en LP/HP, son gain monte avec Q (+80 dB à Q 10 000). Élargir demanderait un filtre à gain compensé (travail DSP non fait).
- RANDOM garde ses plages de la 1.1.1 (decays tirés à partir de 10 ms, CUTOFF / EQ FREQUENCY jusqu'à 18 000 Hz) pour rester musical.
- Les états et presets stockent des valeurs physiques : un projet ou un `.xpl` existant garde son son. La course normalisée (0–1) des 6 potards élargis change : une automation déjà enregistrée sur eux est décalée.

## 3. Règles de conversion, champ par champ

Ordre et clés des champs (indices 0–23) : `OscFreq, OscWave, OscAtk, OscDcy, ModMode, ModAmt, ModRate, ModVel, Mix, NFilMod, NFilFrq, NFilQ, NEnvAtk, NEnvDcy, NEnvMod, NStereo, DistAmt, EQFreq, EQGain, Level, OscVel, NVel, Pan, Choke`.

| Microtonic | πToniC | Règle |
|---|---|---|
| OscFreq | `freq` 20–20 000 Hz | identique |
| OscWave | `wave` 0–100 % | Sine 0, Triangle 33,333…, Saw 66,666… (100/3, 200/3) |
| OscAtk | `attack` 0–10 000 ms | identique |
| OscDcy | `decay` 1–10 000 ms | identique |
| ModMode | `modmode` | Decay 0, Sine 1, Noise 2 (RANDOM) |
| ModAmt | `modamt` ±96 st | identique ; le moteur limite déjà Sine / Random à ±48 st comme Microtonic |
| ModRate | `modrate` 0–20 000 | identique ; `inf` et > 20 000 ms → 20 000 (limité) |
| ModVel | `modvel` 0–200 % | identique ; absent → 0 |
| Mix | `mix` 0–100 % (part du bruit) | ratio `a/b` → `100·b/(a+b)` ; nombre seul (part de l'oscillateur, sérialisation Patternarium) → `100 − x` |
| NFilMod | `filter` | LP 0, BP 1, HP 2 |
| NFilFrq | `cutoff` 20–20 000 Hz | identique |
| NFilQ | `res` 0,1–20 | **ratio** : `q` borné à 0,1–10 000 puis `0,1 + (q − 0,1) / 9 999,9 × 19,9` |
| NEnvAtk | `natk` 0–10 000 ms | identique |
| NEnvDcy | `ndcy` 1–10 000 ms | identique |
| NEnvMod | `env` | Exp 0, Linear 1, Mod 2 (joué en CLAP : 3 redéclenchements fixes de 10 ms) |
| NStereo | `stereo` | Off 0, On 1 ; absent → Off |
| DistAmt | `drive` 0–50 dB | `x × 0,5` ; absent → 0 (algorithme de saturation différent) |
| EQFreq | `eqfreq` 20–20 000 Hz | identique ; absent → 1 000 |
| EQGain | `eqgain` ±40 dB | identique ; absent → 0 |
| Level | `level` 0–100 % | voir § 4 ; `-inf` → 0 % |
| OscVel | `ovel` 0–200 % | identique ; absent → 100 |
| NVel | `nvel` 0–200 % | identique ; absent → 100 |
| Pan | `pan` ±100 | identique ; absent → 0 |
| Choke | `choke` | Off 0, On 1 (GROUP 1) ; absent → Off |

Exemples RESONANCE : Q 0,707 → 0,1012 ; 5 → 0,1098 ; 108 → 0,315 ; 336 → 0,768 ; 3 062 → 6,19 ; 5 581,6 → 11,207 ; 10 000 → 20.

Champs obligatoires : tous sauf ceux qui ont une valeur « absent → ». Option inconnue → refus (`Sound n: unknown <clé>`).

## 4. Niveaux

Microtonic : −∞ à +10 dB (les fichiers générés dépassent parfois +10, jusqu'à +16 dB). πToniC : `level` 0–100 % linéaire.

- Atténuation commune : `att = max(0, max des Level du kit)`.
- Pour chaque voix : `level = 100 × 10^((Level − att) / 20)`.
- Les écarts entre voix sont conservés ; seul le niveau global baisse quand un son dépasse 0 dB. Pas de ratio possible (borne −∞).

## 5. Lecture des nombres et précision

- Nombres : décimal signé avec exposant optionnel ; `inf`, `+inf`, `-inf` acceptés (`inf` seulement pour ModRate, `-inf` seulement pour Level).
- Calcul en double, puis chemin décimal identique à l'état sauvegardé : `%.9g` → `strtof`. Un kit importé est ainsi identique au bit près au `.xpl` converti du même fichier.
- Piège rencontré : les bornes de `fields.h` sont des `float` (0,1f ≠ 0,1). Pour le ratio de RESONANCE, le C relit les bornes en décimal (`%.7g` → `strtod`) pour obtenir les mêmes valeurs que Python. À reproduire dans FM-1 si ses bornes sont en float.
- Tolérances (reprises de FM-1) : indentation libre, espaces ignorés dans les lignes de pas, `Choke` et options absentes, vélocités jusqu'à 200 %.

## 6. Compteur « values limited »

Incrémenté quand une valeur est ramenée à une borne : hors plage Microtonic documentée, `inf`, ou hors plage πToniC après conversion. Le ratio de RESONANCE n'est pas compté (c'est une conversion, pas une limite), sauf si le Q d'origine sort de 0,1–10 000.

Sur 33 fichiers analysés (264 voix) : avant 1.1.2, 71 résonances et 88 gains d'EQ étaient limités ; en 1.1.2, il ne reste que les ModRate infinis ou > 20 s (3 voix).

## 7. Séquences

Inchangées en 1.1.2 et non utilisées par πToniC. Le lecteur de séquences de πClideaN 1.4.0 (même `mtpreset.h`, partie `MT_NO_SOUNDS`) est vérifié identique au lecteur de FM-1 sur 24 cas : motifs A–L, longueur, Triggers / Accents, StepRate, Mutes ; motif invalide = indisponible ; import d'un motif seul ou d'une paire consécutive (A+B … K+L) jusqu'à 32 pas.

## 8. Banque Patternarium BEST

- `scripts/regenerate_patternarium_best.py` reconvertit les 150 kits déjà tirés (`SELECTION.json`, `DRAW.json`) sans nouveau tirage : mêmes sources, ordre, noms et fichiers.
- Sauvegardes privées : `Private/PatternariumBest150-1.1.1/` (`.xpl` et rapports 1.1.1) et `Private/PatternariumBest150-sources/` (150 `.mtpreset` complets avec séquences, numérotés dans l'ordre de la banque, `MANIFEST.json`), pour la future banque de πClideaN.

## 9. À reporter dans FM-1

1. Élargir les plages DECAY, NOISE DECAY (1 ms), CUTOFF, EQ FREQUENCY (20 kHz) et EQ GAIN (±40 dB) si le moteur de FM-1 les supporte ; vérifier la borne interne de ses filtres.
2. Remplacer la limite de RESONANCE par le ratio du § 3, avec les bornes lues en décimal.
3. Garder `inf` → 20 s pour ModRate et l'atténuation commune des niveaux.
4. Rejouer la comparaison valeur par valeur sur les 150 sources de `Private/PatternariumBest150-sources/` et sur les fichiers de test.
