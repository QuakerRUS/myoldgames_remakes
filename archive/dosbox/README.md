# Running the originals in DOSBox

The configurations here start each original program in DOSBox.

| Config | Program | Copy this folder |
|---|---|---|
| `soldier-1.0.conf` | Soldier 1.0 (`SOLDIER.EXE`) | `archive/soldier-1.0/release/` |
| `soldier-1.1.conf` | Soldier 1.1 (`SOLDIER.EXE`) | `archive/soldier-1.0/release/`, then `archive/soldier-1.1-patch/patch/` copied over it, replacing files |
| `soldier-2.0-editor.conf` | Soldier 2.0 editor (`EDITOR/EDITOR.EXE`) | `archive/soldier-2.0/` |
| `piton.conf` | Piton (`PITON.EXE`) | `archive/piton/` |
| `diplom.conf` | Keyboard simulator, 2007 (`diplom.exe`, needs `dos4gw.exe` next to it) | `archive/diplom-2007/program/` |

## Steps

1. Install [DOSBox Staging](https://www.dosbox-staging.org/) or DOSBox 0.74.
2. Copy the program folder from the table to a scratch place outside the repository.
   Never run a program from `archive/` itself: the originals write saves and records into
   their own folder.
3. From that copy, run `dosbox -conf <path to the config>`. Each config mounts the current
   folder as drive `C:` and starts the program.

The original speed settings (GAME SPEED, REPLAY SPEED) are loop counts; adjust them in the
game menu if the pace is wrong under your `cycles`.
