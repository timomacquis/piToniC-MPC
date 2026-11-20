# Construire πToniC — référence 1.0.4

Guide courant au **20 novembre 2026**. Linux/WSL, Python 3/Pillow et chaîne ARM hard-float `πClideaN/toolchain/root` ; conserver les liens Linux. Depuis la racine de πToniC :

```sh
NO_DEPLOY=1 bash scripts/release_vst2.sh
```

Le script lance `scripts/test.sh`, génère le skin, compile le moteur, puis `build_factory_programs.py` pour embarquer les **150 programmes Patternarium BEST**. Il assemble ensuite le dossier SDK dans `build/vst2` et un ZIP ARMv7 intermédiaire dans `dist`.

Avec `NO_DEPLOY=1`, rien n’est installé. Sans cette option, la phase de déploiement sauvegarde et remplace le dossier installé puis relance MPC : projet enregistré et autorisation nécessaires. Le binaire de banque final est celui de `Banks/PatternariumBest150/Compiled/`, pas un ancien moteur sans programmes.

Tests : moteur et vrai wrapper, état, presets, effets, activité, SAMPLE et LCD. Une baseline privée absente ne doit pas être comptée comme un test réussi. Ne pas substituer des binaires archivés aux résultats du build courant.

[Installation](INSTALLATION.md) · [Gen2 non testé sur appareil](INSTALLATION-GEN2.md). La release publique actuelle utilise **`piToniC-1.0.4-mpc-sdk.zip`**, disponible sous `Releases/1.0.4`.

Avant une prochaine publication, adapter `scripts/publish_release.sh` au ZIP unique Gen1 + Gen2 avec `Outils/make_sdk_zip.py`, selon [PUBLICATIONS.md](../../Documentation/Maintenance/PUBLICATIONS.md). Cette actualisation ne modifie aucun script, ne reconstruit aucun paquet et n’ajoute aucune qualification matérielle.
