# Build from source

Requirements: Python 3, Pillow, a native C compiler and an ARMv7 hard-float GCC/sysroot with pthread, libm and libdl. Wrapper and packaging sources are included; their provenance records identify upstream revisions and local changes.

Set `PITONIC_SYSROOT` to the toolchain root containing `usr/bin/arm-linux-gnueabihf-gcc-12`. Adapt the compiler path in the build scripts if your toolchain uses a different name. The bundled code targets 44.1 kHz and ARMv7 NEON.

From the repository root:

```sh
python3 -m venv build/venv
build/venv/bin/pip install Pillow
build/venv/bin/python scripts/generate.py
build/venv/bin/python scripts/build_skin.py
PITONIC_SYSROOT=/path/to/toolchain build/venv/bin/python scripts/build.py
PITONIC_SYSROOT=/path/to/toolchain build/venv/bin/python scripts/build_factory_programs.py
mkdir -p Validation/sample-1.0.1
build/venv/bin/python scripts/package_sample.py
```

The factory build produces PC and ARMv7 binaries and a hash receipt under `Banks/PatternariumBest150/Compiled`. Packaging verifies that receipt and the source inputs, creates the standard installer archive and checks its CRC and per-file SHA-256 hashes.

The archive is written to `dist/1.0.1`. Copy the ZIP and its `.sha256` file to your release assets. Do not run an installer on a development PC.

## Tests

```sh
sh scripts/test.sh
build/venv/bin/python tests/skin_controls_test.py
gcc -std=c11 -g -O1 -fsanitize=address,undefined -fno-omit-frame-pointer \
  -DPITONIC_FACTORY_PROGRAMS=1 -IBanks/PatternariumBest150/Generated \
  -Ibuild -Isrc -Ithird_party/mpc-wrapper/wrapper \
  tests/factory_programs_test.c src/engine.c -lpthread -lm -ldl -o build/factory-test
ASAN_OPTIONS=detect_leaks=1 build/factory-test
```

Offline tests cover voice routing, MIDI offsets, state parsing, effect stability, tempo, activity, native programs and Q-Link document structure. They do not replace listening, touch/Q-Link checks or installation/removal tests on the intended hardware.

Keep existing parameter indices unchanged. Add new parameters at the end of `params.lock`.
