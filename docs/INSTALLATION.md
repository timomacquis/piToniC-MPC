# Installation and removal

## Requirements

Gen1 ARMv7 standalone hardware with MPC OS 3.x, standalone VST support and SSH access. The current package is not an ARM64 / Gen2 build or an MPC OS 2.x skin.

The standard destination is `/sdcard/Synths/Timo - VST - πToniC`. A different writable Synths folder can be selected with the installer. External drives must allow executable plugin binaries.

## Install

1. Save the current project.
2. Extract `piToniC-1.0.0-Patternarium-BEST150-mpc-armv7.zip`.
3. Verify the archive checksum against its `.sha256` file.
4. Copy the extracted package to the device.
5. As root, enter its directory and run:

```sh
sh install.sh
```

The installer checks the payload, backs up the affected settings, copies the instrument and registers it. Read its prompts; installation restarts MPC.

For another Synths destination:

```sh
sh install.sh -t /media/YourDrive/Synths
```

Open a plugin track, select πToniC and choose a sound from PRESET. The built-in bank also has XPL copies under the Force user instrument preset folder. Existing user presets are not overwritten by the bank-copy step.

## Remove

Save the project, enter the same extracted package directory, then run:

```sh
sh uninstall.sh
```

Use the same `-t` destination if you installed to another Synths folder. The uninstaller removes the plugin registration and packaged content and relaunches MPC according to its prompts. Back up user-created presets and projects before removing anything. User preset copies outside the plugin folder can remain in the browser; they do not keep the instrument installed.

## Recovery

Keep the previous working package and the installer's backup. An installer backup is a recovery aid, not a substitute for saving a project. Never remove or replace MPC settings wholesale without checking which other plugins they register.
