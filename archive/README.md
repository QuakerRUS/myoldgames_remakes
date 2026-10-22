# Archive of the originals

The original programs, stored byte-for-byte as they were found in the source zips.
Nothing here is edited; the remakes live elsewhere in the repository.

| Folder | What it is |
|---|---|
| `soldier-1.0/` | Soldier 1.0 (QBasic 4.5, 2000). |
| `soldier-1.1-patch/` | The 1.1 patch and its sources; see [its README](soldier-1.1-patch/README.md). |
| `soldier-2.0/` | An unfinished 2001 rewrite of Soldier. |
| `piton/` | Piton (2000), an unfinished snake game. |
| `diplom-2007/` | The 2007 thesis program "Клавиатурный симулятор слепой печати" (touch-typing simulator). |
| `dosbox/` | DOSBox configurations to run the originals; see [its README](dosbox/README.md). |

## soldier-1.0/

Soldier 1.0, written in QBasic 4.5 in 2000.

- `release/` is the distribution, the contents of `soldier.zip`. Run `SOLDIER.EXE`.
- `workdir/` is the author's working copy of the game folder, with saves and recorded runs:
  `GAMESAV0.DAT` (player LISIY), `GAMESAV1.DAT` (the second player),
  `RECORDS/REC0*.DAT` (runs by LISIY) and `RECORDS/REC1*.DAT` (runs by the second player).
- `src/` holds the BASIC sources: `SOLDIER.BAS` (intro), `MENU.BAS` (menu, players) and
  `GAME1.BAS` (the game).

## soldier-1.1-patch/

The 1.1 patch (`patch/`) and the 1.1 sources (`src/`). See
[soldier-1.1-patch/README.md](soldier-1.1-patch/README.md) for how to apply it and what it changes.

## soldier-2.0/

An unfinished rewrite from 2001: an intro (`SOLDIER.BAS`, `FIRMNAME.BAS`), a working
256-colour palette editor (`PALETTE/`), and an editor shell (`EDITOR/`) in which only the
sprite editor exists, without saving. The data folders (`EDITOR/EPISODES`, `LEVELS`,
`OBJECTS`, `SPRITES`) are empty.

## piton/

Piton (2000), an unfinished snake game. `GRAPHICS/` holds the sprites and
`GRAPHICS/PAINTER2.EXE`, a sprite editor. The game loop in `PIT_BAS/PITON.BAS` stops mid-way.

## diplom-2007/

The 2007 thesis program "Клавиатурный симулятор слепой печати" (Watcom C++ for DOS/4GW).

- `program/` is the runnable build: `diplom.exe` with `dos4gw.exe`, its source `diplom.cpp`,
  and `Data/` (`diplom.dat`, images, `Texts/text_rus.txt`).
- `for-diplom/` holds the key-coordinate tool `input.cpp` / `INPUT.exe`, the images
  (`Images/*.bmp`, `Images/*.psd`) and the palette `Images/palette.act`.

The thesis documents (the thesis text, the presentation, the practice report and the
reviews) are intentionally not stored.

## src-utf8/

Every folder with `.BAS` or `.cpp` sources has a sibling `src-utf8/` folder with UTF-8 copies
for reading. The originals are in CP866 (BASIC) and CP1251 (the C++ comments); the copies
keep the file names and the CRLF line endings.

## MANIFEST.sha256

`MANIFEST.sha256` lists the SHA-256 of every original file (the `src-utf8/` copies,
`.gitkeep` files and these READMEs are not listed). To check the archive against the source
zips:

```
python -I tools/import/build_archive.py --source E:/Archive/Victor/Work --dest archive --verify
```

## Running the originals

See [dosbox/README.md](dosbox/README.md).
