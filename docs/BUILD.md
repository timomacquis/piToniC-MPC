# Construire πToniC — Gen1

Base actuelle : 1.0.3. Linux/WSL, Python 3 et Pillow, compilateur ARM hard-float. `PITONIC_SYSROOT` permet de choisir le sysroot ; le poste local réutilise `πClideaN/toolchain/root`.

Depuis la racine du projet :

```sh
python3 scripts/generate.py
# Employer ici un Python avec Pillow.
python3 scripts/build_skin.py
python3 scripts/build.py
python3 scripts/build_factory_programs.py
python3 scripts/package_sample.py
```

La banque `Banks/PatternariumBest150` et ses sources/attributions doivent être présentes. Le paquetage vérifie les empreintes des entrées, du moteur et du skin : si elles diffèrent, reconstruire les sorties correspondantes au lieu de contourner le contrôle.

Les tests sont dans `scripts/test.sh` et `tests/` : moteur/wrapper, effets, presets, Q-Links SAMPLE et LCD. Certaines comparaisons audio privées exigent leur baseline conservée ; leur absence ne doit pas être déclarée comme un test réussi.

Le ZIP final est dans `dist/1.0.3/`. Les archives déjà publiées restent inchangées lors d’une simple mise à jour documentaire. Aucun script de compilation ne vaut autorisation d’installer ou de relancer MPC.

[Architecture](IMPLEMENTATION.md) · [Publication commune](../../Documentation/Maintenance/PUBLICATIONS.md).


[Gen2 ARM64 build and validation scope](INSTALLATION-GEN2.md).
