# Gen2 ARM64 — πToniC 1.0.3

Compiled for 64-bit Gen2 MPC devices. **Not tested on Gen2 hardware.** Force Gen1 ARM32 remains available and unchanged.

Same engine sources, fixed parameter indices and 150 Patternarium BEST presets as Gen1. Small non-button Image assets receive the padding previously used for πClideaN on Live III; this adaptation still needs verification for πToniC.

PC checks passed: engine, actual wrapper, state round-trip, activity, effects and sample tests under ASan/UBSan; ELF64 AArch64 and GLIBC requirements (maximum 2.34); portable catalogue package and payload SHA-256 checks. No Gen2 device, install/uninstall or CPU validation is claimed.

Download `piToniC-1.0.3-mpc-aarch64.zip` from release v1.0.3. Use its included installation guide and save projects first. Report device/OS and sound, presets, Q-Links, samples and effects results.

## Rebuild

Prepare the normal Gen1 build first (`generate.py`, `build.py`, `build_factory_programs.py`) and retain its receipts and released ARMv7 ZIP in `dist/1.0.3/`. Then run `scripts/build_aarch64.py` with Python/Pillow and an extracted GCC 12 AArch64 toolchain/sysroot in `PITONIC_AARCH64_SYSROOT`. The script verifies engine inputs against the factory build receipt, reuses the released Gen1 skin, pads only small Image components and generates/checks the ARM64 ZIP. No device is contacted.
