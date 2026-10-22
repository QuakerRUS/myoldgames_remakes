# Remakes: Soldier, Piton, Keyboard Simulator — design

Date: 2026-10-22. Author: Fedoseyenko Victor (QuakerRUS).

## Goal

Remake three old projects for Windows and keep them, together with the originals,
in one private repository `QuakerRUS/remakes`.

Guiding principle: **the presentation is far better than the original, while the
functionality and the levels keep their original content.** Every caption and logo that
was drawn with lines (DRAW/CIRCLE) is redrawn as proper graphics.

Sources of the originals (read-only): `E:\Archive\Victor\Work` —
`QBasic\QBasic.zip`, `QBasic\Soldier\{soldier,patch1_1,source1_1}.zip`, `Diplom.zip`.

## Common rules

- Kulakov Taras is removed from the remakes' credits. They read "Program by Fedoseyenko Victor".
- Company: **V Software Entertainment**; place and year: **Russia, Moscow, 2026**. No phone numbers.
- The sources of graphics and sound are not mentioned in the games. Free assets (CC0) are
  used; whatever is missing is drawn or synthesized; one consistent style per game.
- UI languages follow the originals: Soldier and Piton in English, Keyboard Simulator in Russian.
- Manual speed tuning (GAME SPEED, REPLAY SPEED, PITON.CFG) is replaced by real-time timing.
- The originals in `archive/` are stored byte-for-byte, including Taras's name and demo files.
- All documentation in the repository is in English.

## Repository layout

```
remakes/
├─ README.md
├─ CMakeLists.txt                top-level project; dependencies via FetchContent
├─ archive/                      originals, byte-for-byte
│  ├─ soldier-1.0/
│  │  ├─ release/                contents of soldier.zip
│  │  ├─ workdir/                the SOLDIER folder from QBasic.zip (with saves and demos)
│  │  └─ src/                    SOLD_BAS in CP866 + src-utf8/
│  ├─ soldier-1.1-patch/
│  │  ├─ patch/                  contents of patch1_1.zip
│  │  ├─ src/                    contents of source1_1.zip in CP866 + src-utf8/
│  │  └─ README.md               how to apply the patch and what it changes
│  ├─ soldier-2.0/               SOLDIER.NEW (unfinished) + src-utf8/
│  ├─ piton/                     PITON from QBasic.zip + src-utf8/
│  ├─ diplom-2007/               the program only, without the thesis documents (.doc/.ppt)
│  └─ dosbox/                    DOSBox configs to run the originals
├─ engine/                       shared library
├─ tools/
│  ├─ painter/                   graphics editor
│  └─ import/                    converters of the old data (Python)
├─ soldier/{game,editor,data}/
├─ piton/{game,editor,data}/
├─ keyboard-simulator/{app,data}/
└─ docs/                         specs, file formats, analysis of the originals
```

`archive/diplom-2007/` holds `diplom.cpp`, `diplom.exe`, `dos4gw.exe`, `Data/`
(diplom.dat, bg.bmp, fonts.bmp, text_rus.txt), `input.cpp`, `INPUT.exe`, `palette.act`,
`Images/*.psd`. Not included: `Diplom.doc`, `Diplom.ppt`, `Practic.doc`, the supervisor's
and the reviewer's reports.

## Technology

- C++20, MSVC 2022, CMake ≥ 3.28, one-command build `cmake --preset release`.
- SDL3 (window, input, audio device, rendering), stb_image / stb_image_write (PNG),
  stb_truetype or SDL3_ttf (fonts), stb_vorbis (OGG), Dear ImGui (editor UI).
  All fetched through FetchContent at pinned versions.
- Python 3.12 — only for the converters in `tools/import/`.
- Resizable window, base resolution 1280×720, fullscreen mode, no filtering for pixel art
  (integer scaling).

## Engine (`engine/`)

A small static library shared by all programs. Modules:

| Module | Purpose |
|---|---|
| `app` | window, main loop with a fixed logic step and free-running rendering, scene switching |
| `gfx` | textures, sprite atlases, animations (frames + durations), camera, text |
| `audio` | mixer: sound effects (WAV/OGG), looping music, volume |
| `input` | keyboard and mouse, key repeat; no rebinding |
| `ui` | in-game menus, text fields, Yes/No dialogs |
| `storage` | data and save paths (`%APPDATA%\V Software\<game>`), JSON |
| `editor` | common base of the map editors: grid, object palette, undo/redo, file handling |

Game logic is separate from rendering: the level model and the rules are plain C++
without SDL and are covered by unit tests (doctest).

## Soldier 2.0

### Content (unchanged from 1.1)

- The 51 levels of patch 1.1. A time limit remains only on levels 1, 24, 33 and 34
  (300, 120, 36 and 15 timer units, see below); the other levels have none.
- The field is a grid of tiles; each level has its own size (up to 100×100).
- The rules are an exact port of `GAME1.BAS` version 1.1:

| Code | Tile | Behaviour |
|---|---|---|
| 0 | floor | walkable |
| 1 | wall | blocks |
| 2 | exit | stepping on it wins the level |
| 3–6 | player on floor (down/up/left/right) | level start |
| 8 | box | can be pushed if the cell behind it is floor, a button, an open door, water or a sunk box |
| 9 | button | pressed while the player, a box or a key is on it |
| 10 | box on button | |
| 11–14 | player on button | |
| 15 | door | open only while every button of the level is pressed |
| 16–19 | player in door | |
| 20 | box in door | |
| 21–44 | 6 coloured keys and doors: per colour a key, a door, a key on a button, a key in a door | a key goes to the inventory (up to 25); a coloured door opens with a key of its colour when entered; Space/Enter puts the selected key in front of the player or opens the door in front |
| 45 | dark wall | turns into a plain wall when the player tries to pass |
| 46 | fake wall (looks like a dark wall) | walkable |
| 48 | guillotine | closed during the 4th quarter of every second; cannot be entered while closed; a player inside when it closes dies |
| 49–52 | player under guillotine | |
| 54 | water | stepping into it kills; a box pushed into water sinks and becomes floor |
| 56 | sunk box | walkable |
| 57–60 | player on sunk box | |
| 63 | box on sunk box | |
| 7, 47, 53, 55, 61, 62 | death animations | gave up (Esc), time out, guillotine, drowned, etc. |

- Controls as in the original: arrows step/turn, `,` `<` and `.` `>` turn counter-clockwise
  and clockwise, Tab selects the next inventory item, Space/Enter uses the item,
  Esc gives up.
- The rhythm is kept in ticks: a game "second" is 4 ticks, a step takes 2 ticks, the
  guillotine is closed on the 4th tick of the "second", the level timer decreases once
  every 4 ticks. This is needed for the timing puzzles and for replaying the old demos.
- The pace is twice as slow as the original's "real second": tick 500 ms, step 1 s.
  The timer shows the original numbers (300, 120, 36, 15) and, as in the original,
  decreases by one every 4 ticks — at ×1 that is 2 real seconds, so the limit is doubled.
- Game speed is the GAME SPEED setting in OPTIONS: 5 steps ×0.5, ×0.75, **×1 (default,
  tick 500 ms)**, ×1.5, ×2. Only the tick length changes; all logic and the timer run in
  ticks, so the timer numbers do not depend on the speed (×2 is the original pace),
  and neither do the recordings.
- Obvious bugs of the original are fixed: when pushing a box to the right, the sunk-box
  check looked to the left (`ml(absx - 2, absy)` in subroutines 2300 and 5600); the
  "hang the computer" routine on tampered saves is removed.

### Presentation

- 32×32 tiles drawn with integer scaling. The camera shows as much of the field as fits
  and smoothly follows the player.
- Animations: walking and turning, pushing a box, a button pressing in, a door opening,
  picking up a key, the guillotine going down and up, flowing water, a box sinking,
  each kind of death.
- A sound for every action, music in the menu and in the game, faster music during the
  last 15 units of a time limit.
- Side panel: level, timer (green > 59, yellow 15–59, red < 15), 5×5 inventory with a
  cursor. The HEALTH/ARMOR/AMMO lines of the original (always 100/0/0) are dropped.
- Intro: animated logo "V Software Entertainment presents", then "SOLDIER",
  "Program by Fedoseyenko Victor", "Russia, Moscow, 2026", version 2.0. Any key skips.

### Menu and saves

- 10 player slots with a name (up to 8 characters, A–Z, 0–9) and the level reached.
  Tab or the arrows select a player.
- Items: NEW GAME (enter a level not above the one reached), REPLAY LEVEL (show the best
  run of a completed level), DELETE PLAYER (with confirmation), OPTIONS (game speed,
  sound and music volume, fullscreen), EXIT.
- Saves are JSON, without tamper protection.
- A run is recorded as a list of (tick, key); on a win it becomes this player's record
  for the level. Replay: speed ×1/×2/×4, any key exits.
- The built-in player LISIY with his level reached and his `REC0*.DAT` records
  (converted from the old format). Taras's demos and save are not carried over.
  After level 51 — a victory screen.

### Data import

`tools/import/` converts:
- `LEVELn.DAT` (header: 4 × int16 — width, height, time, number; then width×height
  bytes of codes, row by row) → `soldier/data/levels/NN.json`;
- `REC0nn.DAT` (int32: seed, speed, then triples of int32 tick number, iteration inside
  the tick, 2 bytes of key) → recordings in the new format; each one is verified by
  running the logic until the exit;
- the old 16×16 sprites (`GRAPHICS/*.DAT`, 8-bit, standard VGA palette) → PNG as a
  reference for the new graphics.

### Soldier map editor

Modelled on the PAINTER2/EDITOR interface of Soldier 2.0:
- the field grid on the left, the object palette on the right (every tile from the table);
  Tab switches the panel, the arrows move the frame cursor, Space/Enter places,
  Backspace resets to floor; mouse: LMB places, RMB erases, wheel zooms, middle button pans;
- level size, time limit (0 = none), number;
- undo/redo, new/open/save, Esc asks "Save? Yes/No/Cancel";
- level check: exactly one player, an exit exists;
- test-play the level in the game with one key (F5).

## Piton

### Game

- A 38×23 field, a snake of 3 segments, arrows set the direction, a 180° turn is not
  allowed; hitting a border, a brick or itself is GAME OVER.
- A rabbit appears on a random free cell; eating it makes the snake longer.
- 10 levels with brick walls, growing in difficulty. **Each level requires eating
  50 rabbits**, then the next level starts. After the 10th — a victory screen.
- One speed, real-time.
- Menu: NEW GAME, DEMO, EXIT. In DEMO the computer drives the snake around walls and
  its own body (pathfinding); levels go in order.
- Only the level reached and the high score are saved; NEW GAME offers to start on any
  unlocked level.

### Presentation

- An animated snake: the head in several phases per direction, body, turns, tail — after
  the original sprite set, at the new resolution. An animated rabbit. New grass and
  brick textures.
- Sounds (eating, turning, death, level complete), music.
- The same company intro, then "PITON".

### Piton map editor

On the shared `engine/editor` base: a 38×23 field; objects are brick, empty, and the
snake's start position and direction. Same keys, mouse, undo/redo, test-play.

## Painter (graphics editor)

Shared by Soldier and Piton, in the spirit of PAINTER2:
- a pixel grid and a palette; cursor by keyboard and mouse;
- tools: pencil, eraser (transparent), fill, line, rectangle, picker;
- animation frames, animation preview with a per-frame duration;
- opens and saves PNG and frame sheets with an animation description (JSON next to the PNG);
- undo/redo.

## Keyboard Simulator

A remake of the 2007 thesis program "Клавиатурный симулятор слепой печати"
(touch-typing simulator).

- One window in a modern design with the layout from the thesis:
  - the text field on top: the current and the next sentence, word wrap, the current
    letter marked by a cursor;
  - a stats line: speed (characters per minute), errors, accuracy;
  - buttons: Рестарт, Пауза, Помощь, О программе (Restart, Pause, Help, About);
  - an on-screen ЙЦУКЕН keyboard below, keys coloured by finger, with the finger legend
    "5 4 3 2 1 / 1 2 3 4 5"; the key to press is highlighted, and for capitals and
    shifted symbols the right Shift is highlighted too.
- Behaviour as in the thesis: a correct key moves the cursor; a wrong key counts as an
  error and briefly flashes; a space is required at the end of a line; after `.`, `!`,
  `?` the sentence leaves, the next one moves up, and a new random one is added below.
- Input by the actual Russian character (SDL text input), not an emulation on top of QWERTY.
- Speed is measured in real time with millisecond precision; Pause stops the clock,
  Restart resets the stats.
- Texts: the same 5629 sentences from `text_rus.txt`, converted from CP866 to UTF-8.
- Bugs of the original are fixed: the capital "Е", the shifted digits, the impossible "0".
- No user profiles, lessons or statistics history.
- About: Keyboard Simulator, Program by Fedoseyenko Victor,
  V Software Entertainment, Russia, Moscow, 2026.

## Verification

- Unit tests of the logic: Soldier's rules for every tile, every LISIY demo replayed to
  the exit, Piton's rules (growth, collisions, rabbits only on free cells), speed and
  error counting in Keyboard Simulator.
- Every Piton level is checked for solvability (all free cells reachable).
- A manual check of every program before a stage is handed over.

## Stages

1. Repository, archive of the originals, README, CMake skeleton.
2. Engine and Soldier (game), then the map editor and Painter.
3. Piton: game, 10 levels, map editor.
4. Keyboard Simulator.

Each stage gets its own implementation plan and a review by the author.
