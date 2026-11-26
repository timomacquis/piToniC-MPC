#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
python3 scripts/generate.py
CXXFLAGS='-std=c++17 -g -O1 -fsanitize=address,undefined -fno-omit-frame-pointer -Isrc'
FLAGS='-pthread -D_POSIX_C_SOURCE=200809L -std=c11 -g -O1 -fsanitize=address,undefined -fno-omit-frame-pointer -Ibuild -Isrc -Ithird_party/mpc-wrapper/wrapper'
g++ $CXXFLAGS -c src/fx_new.cpp -o build/fx_new-test.o
FXN=build/fx_new-test.o
gcc $FLAGS -c tests/engine_test.c -o build/engine-test-0.o
g++ -fsanitize=address,undefined build/engine-test-0.o $FXN -lm -o build/engine-test
ASAN_OPTIONS=detect_leaks=1 ./build/engine-test
gcc $FLAGS -c tests/host_test.c -o build/host-test-0.o
gcc $FLAGS -c src/engine.c -o build/host-test-1.o
g++ -fsanitize=address,undefined build/host-test-0.o build/host-test-1.o $FXN -lpthread -lm -ldl -o build/host-test
ASAN_OPTIONS=detect_leaks=1 ./build/host-test
gcc $FLAGS -c tests/activity_test.c -o build/activity-test-0.o
g++ -fsanitize=address,undefined build/activity-test-0.o $FXN -lm -o build/activity-test
ASAN_OPTIONS=detect_leaks=1 ./build/activity-test
gcc $FLAGS -c tests/effects_test.c -o build/effects-test-0.o
g++ -fsanitize=address,undefined build/effects-test-0.o $FXN -lm -o build/effects-test
ASAN_OPTIONS=detect_leaks=1 ./build/effects-test
gcc $FLAGS -c tests/sample_test.c -o build/sample-test-0.o
g++ -fsanitize=address,undefined build/sample-test-0.o $FXN -lm -pthread -o build/sample-test
ASAN_OPTIONS=detect_leaks=1 ./build/sample-test
gcc $FLAGS -c tests/state_size_test.c -o build/state-size-test-0.o
gcc $FLAGS -c src/engine.c -o build/state-size-test-1.o
g++ -fsanitize=address,undefined build/state-size-test-0.o build/state-size-test-1.o $FXN -lpthread -lm -o build/state-size-test
./build/state-size-test
gcc $FLAGS -c tests/classic_regression.c -o build/classic-regression.o
g++ -fsanitize=address,undefined build/classic-regression.o $FXN -lpthread -lm -o build/classic-regression
# CLASSIC doit rester identique au bit près au moteur du dernier commit, compilé ici avec le même compilateur.
OLD=build/classic-old; rm -rf $OLD; mkdir -p $OLD/src $OLD/tests; cp src/*.h $OLD/src/; git show HEAD:./src/engine.c > $OLD/src/engine.c; cp tests/classic_regression.c $OLD/tests/
gcc $FLAGS -c $OLD/tests/classic_regression.c -o $OLD/cr.o; g++ -fsanitize=address,undefined $OLD/cr.o $FXN -lpthread -lm -o $OLD/cr
A=$(./$OLD/cr classic); N=$(./build/classic-regression classic)
if [ "$A" = "$N" ]; then echo "PASS classic FX byte-identical to HEAD ($N)"; else echo "FAIL classic FX differs from HEAD: avant $A, après $N"; exit 1; fi
gcc $FLAGS -c tests/fxnew_test.c -o build/fxnew-test.o
g++ -fsanitize=address,undefined build/fxnew-test.o $FXN -lpthread -lm -o build/fxnew-test
ASAN_OPTIONS=detect_leaks=1 ./build/fxnew-test
# 1.1.1 : import de kit MicroTonic .mtpreset (lecteur, moteur, comparaison aux 150 kits si l'archive locale est présente).
gcc $FLAGS -c tests/mtpreset_test.c -o build/mtpreset-test.o
g++ -fsanitize=address,undefined build/mtpreset-test.o -lm -o build/mtpreset-test
ASAN_OPTIONS=detect_leaks=1 ./build/mtpreset-test
gcc $FLAGS -c tests/kit_import_test.c -o build/kit-import-test.o
g++ -fsanitize=address,undefined build/kit-import-test.o $FXN -lpthread -lm -o build/kit-import-test
ASAN_OPTIONS=detect_leaks=1 ./build/kit-import-test
ASAN_OPTIONS=detect_leaks=0 python3 tests/mtpreset_compare.py build/mtpreset-test
# 1.2.0 : source WAVETABLE (tables, balayage, SOURCE à 3 valeurs, état WT1, compatibilité 1.1.2, import .mtpreset).
gcc $FLAGS -c tests/wavetable_test.c -o build/wavetable-test.o
g++ -fsanitize=address,undefined build/wavetable-test.o $FXN -lpthread -lm -o build/wavetable-test
ASAN_OPTIONS=detect_leaks=1 ./build/wavetable-test
gcc $FLAGS -c tests/wavetable_host_test.c -o build/wavetable-host-test-0.o
gcc $FLAGS -c src/engine.c -o build/wavetable-host-test-1.o
g++ -fsanitize=address,undefined build/wavetable-host-test-0.o build/wavetable-host-test-1.o $FXN -lpthread -lm -ldl -o build/wavetable-host-test
ASAN_OPTIONS=detect_leaks=1 ./build/wavetable-host-test
