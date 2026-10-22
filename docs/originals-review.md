# Review of the originals

A factual summary of what the original programs were, how they store their data, how the
code is built, which bugs were found, and how the remakes treat each point. Everything here
was checked against the files in `archive/` (UTF-8 copies of the sources in the `src-utf8/`
folders) and the conversion scripts in `tools/import/`.

## What the originals are

| Original | Year, language | Location in `archive/` | State |
|---|---|---|---|
| Soldier 1.0 | 2000, QBasic 4.5 (compiled) | `soldier-1.0/` (`release/`, `workdir/`, `src/`) | complete game: 51 levels, players, recorded runs |
| Soldier 1.1 patch | 2000, QBasic 4.5 | `soldier-1.1-patch/` (`patch/`, `src/`) | patch over 1.0, see its [README](../archive/soldier-1.1-patch/README.md) |
| Soldier 2.0 | 2001, QBasic | `soldier-2.0/` | unfinished rewrite: intro, palette editor, editor shell |
| Piton | 2000, QBasic | `piton/` (`PIT_BAS/`, `GRAPHICS/`, `PITON.EXE`) | unfinished snake game |
| PAINTER2 | DOS executable, no source | `piton/GRAPHICS/PAINTER2.EXE` | sprite editor, stored with the Piton sprites |
| Keyboard simulator | 2007, Watcom C++ for DOS/4GW | `diplom-2007/` (`program/`, `for-diplom/`) | thesis program "Клавиатурный симулятор слепой печати" |

DOSBox configurations to run them are in `archive/dosbox/`. File checksums are in
`archive/MANIFEST.sha256`.

### Soldier 1.0 and 1.1

A tile puzzle: walk a soldier through 51 levels with boxes, buttons, doors, coloured keys,
guillotines, water and fake walls. Three programs chain into each other with `RUN`:
`SOLDIER.BAS` (intro) → `MENU.BAS` (players, menu) → `GAME1.BAS` (the game, about 1175 lines),
and they pass their state through a file `soldier.tmp`. The menu offers NEW GAME, REPLAY LEVEL,
DELETE PLAYER, GAME SPEED and REPLAY SPEED; the original intro credits "Fedoseyenko Victor &
Kulakov Taras, Russia, Klintsy" with phone numbers.

Patch 1.1 (verified file by file, see its README) removes the time limit from all levels
except 1, 24, 33 and 34 (time field 0 = no limit), shows "LEVEL NOT FOUND. YOU WIN!" after
level 51, changes the version label and the default speeds in `SOLDIER.CFG`.

### Soldier 2.0 (2001)

`SOLDIER.BAS` / `FIRMNAME.BAS` are an intro (with the caption "E N T E R T A I M E N T",
misspelt); `PALETTE/` is a working 256-colour palette editor; `EDITOR/` is an editor shell in
which only the sprite editor exists, without saving. Its data folders (`EPISODES`, `LEVELS`,
`OBJECTS`, `SPRITES`) are empty. The remake's map editors and Painter follow its layout
(field left, palette right, frame cursor, Tab between them).

### Piton (2000)

`PIT_BAS/PITON.BAS` (about 560 lines) has a menu (NEW GAME, DEMO, GAME SPEED), an empty
38×23 field with a border, a snake of 3 segments, a rabbit and a simple demo driver that turns
towards the rabbit. There are no levels; the game ends on hitting the border or the snake
("GAME OVER") and announces "YOU ARE WIN!!!" after 750 rabbits. The main loop stops mid-way.
`GRAPHICS/` holds the sprites (head phases per direction, body, turns, tail, rabbit, grass).

### Keyboard simulator (2007)

`program/diplom.cpp` (about 490 lines, Watcom C++ with `graph.h`, 640×400 graphics mode)
shows a text field with the current and the next sentence (41 columns), the speed, and a
picture of the ЙЦУКЕН keyboard (`bg.bmp`) on which the key to press is outlined. Input is
read with `getch()` on a QWERTY system and translated to the Russian glyph through a table in
`diplom.dat`. `for-diplom/input.cpp` is the tool that recorded the key rectangles.

## Data formats

| Data | Original | Remake | Description |
|---|---|---|---|
| Soldier levels | `LEVELS/LEVELn.DAT`: 4 × int16 header + one byte per tile | `soldier/data/levels/NN.json` | [soldier-level.md](formats/soldier-level.md) |
| Soldier recordings | `RECORDS/REC<slot><level>.DAT`: seed, speed, then (part, iteration, key) | `soldier/data/records/LISIY/NN.json` | [soldier-recording.md](formats/soldier-recording.md) |
| Soldier saves | `GAMESAVn.DAT`: name, level reached, anti-tamper noise | `players.json`, `%APPDATA%\V Software\Soldier\save.json` | [soldier-recording.md](formats/soldier-recording.md) |
| Piton levels | none (empty field only) | `piton/data/levels/NN.json` | [piton-level.md](formats/piton-level.md) |
| Keyboard layout | `Data/diplom.dat` (key rectangles, QWERTY codes) + `bg.bmp` (finger colours) | `keyboard-simulator/data/layout/ru.json` | [kbsim-layout.md](formats/kbsim-layout.md) |
| Sentences | `Data/Texts/text_rus.txt`, CP866 | `keyboard-simulator/data/texts/ru.txt`, UTF-8, 5630 sentences | [kbsim-texts.md](formats/kbsim-texts.md) |

The converters are in `tools/import/` (`soldier_levels.py`, `soldier_records.py`,
`kbsim_layout.py`, `kbsim_texts.py`, and `build_archive.py` for the archive itself).

## Code quality notes

- **Line-numbered GOSUB subroutines.** The BASIC programs are flat: control flow is `GOTO`
  and `GOSUB` to numbered lines (1000, 1100, … 5600 in `GAME1.BAS`), with comments in Russian
  above each subroutine. Similar rules are copied per direction and per tile code (e.g. the
  box-push checks in subroutines 2300 and 5600), which is where the copy-paste bugs below
  come from.
- **Global state.** All state is global: the map `DIM ml(100, 100)`, the inventory, the
  position `absx`/`absy`, the counters `numsec&`, `p&`. The diploma program likewise uses
  global arrays and buffers.
- **Busy-loop timing.** Soldier has no clock: each quarter of a game second is
  `FOR f& = 1 TO gs&` with an inner `FOR ff = 1 TO 250: NEXT ff`, the loop unrolled four
  times, and keys are polled with `INKEY$` inside it. `gs&` (GAME SPEED) and `ls&` (REPLAY
  SPEED) are loop counts set by the player and stored in `SOLDIER.CFG` (1.1 default
  `15000, 2500`); the real pace therefore depends on the CPU. Piton does the same with one
  value in `PITON.CFG`.
- **Graphics.** Sprites are `PUT` arrays loaded from `.DAT` files; logos and frames are
  drawn with `LINE`, `DRAW`, `CIRCLE` and `PAINT`; text is `PRINT`. The Soldier game and Piton
  use `SCREEN 13` (320×200, 256 colours), the Soldier intros `SCREEN 12`, the Soldier 1.x menu
  the 40-column text mode.
- **Anti-tamper code.** `MENU.BAS` checks that every save record `f` equals record `213 - f`
  minus 1, and `GAME1.BAS` checks the level number in the level header; on a mismatch both
  call subroutine 1000, which zeroes the first 32 KB of memory with `DEF SEG = 0: POKE` and
  divides by zero, hanging the computer.
- **Recorder.** Runs are recorded as (part counter, loop iteration, key), so replay depends
  on the GAME SPEED at recording time; the speed is stored in the file.

## Bugs found and how the remakes treat them

### Soldier (1.1)

From `soldier/core/README.md`, "Differences from the original". None of them affects any of
the 51 LISIY demos.

| Bug | Remake |
|---|---|
| Pushing a box (8 or 63) to the right tested `ml(absx - 2, absy)` (two cells to the left) for a sunk box, in subroutines 2300 and 5600 | fixed: the cell behind the box is tested |
| Tab (next inventory item) was unreachable: subroutine 1300 returns for every one-character key before testing Tab | Tab works |
| Using an item while under a guillotine or on a sunk box used a stale target cell | uses the cell in front |
| A bump while standing on a sunk box did not turn the player (subroutine 2100 omits codes 57–60) | turns |
| The level sits at `ml(1..w, 1..h)` in a 0..100 array; the cells around it are floor, and index −1/101 stops the program | outside the map is treated as wall. Reachable only through edge pockets on levels 25 and 28, both behind boxes; in normal play nobody goes there |
| The turn keys and Space were not recorded by the 1.1 recorder (one-character keys return before the recording call) | every executed command is recorded |
| Tampered saves or levels hang the computer (subroutine 1000) | removed; saves are plain JSON |

The rules are otherwise ported exactly, on a discrete tick clock (4 ticks per game second,
a step every 2 ticks), so the 51 LISIY records replay to the exit; the importer verifies each
one against a transliteration of the 1.1 replay loop. The records of levels 34–36 contain
turn and Space keys, which the 1.1 recorder could not write; they replay correctly.

### Piton

The original never got levels or a working end of the loop, so the remake keeps the idea
(38×23 field, 3 segments, arrows, no 180° turn, rabbits) and adds what is missing: 10 brick
levels of 50 rabbits each, scoring, a pathfinding demo, and a level check. The rabbit
subroutine 1500 picks `rx`, `ry` but tests `ml(x, y)` and draws at `(x * 8, y * 8)`; `x` and
`y` are never assigned (always 0), so the chosen cell is never checked against the snake and
the rabbit is drawn in the corner instead of at its cell. The remake places the rabbit on a
uniformly random cell that is empty and not on the snake.

### Keyboard simulator

From `keyboard-simulator/core/README.md`, `docs/formats/kbsim-layout.md` and
`diplom.cpp` (`recodeLetter`; glyph order: А–Я, а–я, space 66, digits 1–9 at 67–75, 0 at 76,
punctuation 77–94):

| Bug | Cause | Remake |
|---|---|---|
| The capital "Е" in a sentence cannot be typed | `recodeLetter` maps CP866 128–132 (А–Д) and 134–165 to glyphs; 133 (Е) falls between the ranges and gets no glyph | input and text are compared as Unicode characters (SDL text input) |
| Digits are shifted by one | a text digit `d` becomes glyph `d + 19`, so `0` (48) becomes glyph 67, which is `1`; every digit is shown and expected as the next one, and `9` as `0` | digits are typed as written |
| `0` (glyph 76) is impossible | in `diplom.dat` glyph 76 records `:` as its QWERTY code instead of `0`; `:` is found first at `Ж`, so glyph 76 is never produced | can be typed; `kbsim_layout.py` reports the wrong code and does not use it |
| Input works only with a QWERTY system layout | key presses are `getch()` QWERTY characters translated through `diplom.dat` | the actual Russian character is used; a Latin letter shows a hint to switch the layout |
| Errors are not counted; the clock has whole-second precision (`_dos_gettime`) | | errors and accuracy are counted; millisecond clock, stopped while paused |
| A word longer than a line hangs the word wrap | | the word is cut |

`keyboard-simulator/core/README.md` lists "the shifted digit-row symbols" among the fixes;
the source shows the digits themselves shifted by one, as above. The remake fixes both cases,
since every character of the layout is matched exactly.

The original stopped a sentence at its first `.` `!` `?`; the remake completes it at its last
character (no sentence of `ru.txt` has one before its end). One Latin `c` inside a Russian
word (line 2631 of `text_rus.txt`) is replaced with the Cyrillic `с`.

## What is not carried over

- Kulakov Taras's name, his save (`GAMESAV1.DAT`) and records (`REC1*.DAT`); they stay in
  `archive/`.
- The HEALTH / ARMOR / AMMO lines of Soldier (always 100 / 0 / 0).
- Manual speed calibration (GAME SPEED, REPLAY SPEED, PITON.CFG); the remakes run in real
  time, Soldier with a GAME SPEED multiplier of the tick length.
- The thesis documents of 2007 (text, presentation, reports), which are not in the repository.
