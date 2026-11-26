#!/usr/bin/env bash
# πToniC 1.2.0-candidate.1 : source WAVETABLE (8 tables, POS / END / SCAN TIME / SCAN), SOURCE à 3 choix.
# A lancer depuis WSL :  bash scripts/candidate_120.sh
#   0. tests PC (test.sh depuis une copie temporaire où l'ancien moteur de la régression CLASSIC est lié avec $FXN,
#      défaut connu de test.sh, inchangé ici), dont tests/wavetable_test.c et tests/wavetable_host_test.c ;
#   1. skin + tests/wavetable_skin_test.py (grille 4 x 2, 64 vues de table, Q-Links TONE) ;
#   2-5. scripts/release_vst2.sh : skin, ARMv7, 150 programmes natifs, dossier SDK, ZIP, sauvegarde de la version
#      en place et installation dans « VST2 Plugins » (Outils/deploy_vst2.sh).
# FORCE_IP=… pour une autre adresse ; NO_DEPLOY=1 construit sans installer. Voir docs/CANDIDATE-1.2.0.md.
# S'arrête au premier échec. Aucun commit.
set -euo pipefail
cd "$(dirname "$0")/.."
export FORCE_IP="${FORCE_IP:-10.167.246.70}"
VER=$(python3 -c "import sys;sys.path.insert(0,'scripts');from config import VERSION;print(VERSION)")
[ "$VER" = 1.2.0 ] || { echo "scripts/config.py annonce $VER, attendu 1.2.0 : arrêt."; exit 1; }

echo; echo "=== 0/5 tests PC ==="
T=$(mktemp); trap 'rm -f "$T"' EXIT
sed -e "s#^cd .*#cd \"$PWD\"#" -e 's#\$OLD/cr.o -lpthread -lm -o \$OLD/cr#$OLD/cr.o $FXN -lpthread -lm -o $OLD/cr#' scripts/test.sh > "$T"
grep -q 'cr.o \$FXN' "$T" || { echo "Correctif temporaire de test.sh non appliqué : arrêt."; exit 1; }
sh "$T"

echo; echo "=== 1/5 skin WAVETABLE ==="
python3 scripts/build_skin.py
python3 tests/wavetable_skin_test.py

SKIP_TESTS=1 bash scripts/release_vst2.sh
[ "${NO_DEPLOY:-0}" = 1 ] && exit 0
echo
echo "Candidat πToniC $VER installé sur $FORCE_IP. Aucun commit créé."
echo "Relancer MPC : ssh root@$FORCE_IP \"systemctl restart acvs\""
