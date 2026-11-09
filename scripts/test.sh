#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
python3 scripts/generate.py
FLAGS='-pthread -D_POSIX_C_SOURCE=200809L -std=c11 -g -O1 -fsanitize=address,undefined -fno-omit-frame-pointer -Ibuild -Isrc -Ithird_party/mpc-wrapper/wrapper'
gcc $FLAGS tests/engine_test.c -lm -o build/engine-test
ASAN_OPTIONS=detect_leaks=1 ./build/engine-test
gcc $FLAGS tests/host_test.c src/engine.c -lpthread -lm -ldl -o build/host-test
ASAN_OPTIONS=detect_leaks=1 ./build/host-test
gcc $FLAGS tests/activity_test.c -lm -o build/activity-test
ASAN_OPTIONS=detect_leaks=1 ./build/activity-test
gcc $FLAGS tests/effects_test.c -lm -o build/effects-test
ASAN_OPTIONS=detect_leaks=1 ./build/effects-test
gcc $FLAGS tests/sample_test.c -lm -pthread -o build/sample-test
ASAN_OPTIONS=detect_leaks=1 ./build/sample-test
