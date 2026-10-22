# Remakes — Soldier, Piton, Keyboard Simulator

Remakes of three old projects by Fedoseyenko Victor (V Software Entertainment) for modern
Windows: the QBasic games Soldier and Piton (2000) and the 2007 thesis program, a touch-typing
simulator. The presentation is redone; the functionality and the levels keep their original
content. The originals are kept byte-for-byte in [`archive/`](archive/README.md).

## Projects

| Project | Original | Remake | Status |
|---|---|---|---|
| Soldier 2.0 | Soldier 1.0/1.1, QBasic, 2000 | the game (51 levels) and a map editor, C++20/SDL3 | done |
| Piton | Piton, QBasic, 2000, unfinished | a finished game with 10 levels, a demo mode and a map editor | done |
| Keyboard Simulator | "Клавиатурный симулятор слепой печати", 2007 thesis program (Watcom C++, DOS) | a modern remake, Russian UI | done |
| Painter | PAINTER2 sprite editor | the pixel-art editor shared by Soldier and Piton | done |

### Soldier 2.0

A tile puzzle: lead the soldier to the exit past walls, boxes, buttons and doors, coloured keys
and doors, guillotines and water. The 51 levels and the rules of Soldier 1.1 (an exact port,
with the original's bugs fixed), new pixel art, animations, sounds and music, 10 player slots,
records of the best runs and their replays, the built-in player LISIY with his demos.

Controls: arrows step / turn, `,` `.` turn, Tab next key in the inventory, Space / Enter use the
key, Esc give up, P pause, Z zoom 2× / 1× (2× by default), F11 / Alt+Enter fullscreen.
Map editor (`soldier_editor.exe`): Tab field ↔ palette, arrows, Space places, Backspace erases,
mouse paints, Ctrl+Z / Ctrl+Y, F5 test-plays the level in the game.
Details: [`soldier/core/README.md`](soldier/core/README.md) (rules),
[`soldier/editor/README.md`](soldier/editor/README.md).

### Piton

A snake game: eat 50 rabbits on each of 10 levels (Open Field … Labyrinth), avoid the walls and
your own tail. NEW GAME starts on any level reached; DEMO lets the computer play.
Controls: arrows / WASD, P pause, Esc menu. Map editor (`piton_editor.exe`): the same editor
keys, plus 1–6 palette, R rotate the start, H / V symmetry painting, F5 test-play.
Details: [`piton/core/README.md`](piton/core/README.md),
[`piton/editor/README.md`](piton/editor/README.md).

### Keyboard Simulator

A touch-typing trainer for the ЙЦУКЕН layout: the current and the next sentence (5630 sentences
of the original), speed, errors and accuracy, an on-screen keyboard coloured by finger that
highlights the key to press and the Shift of the opposite hand.
Controls: type in the Russian layout; F2 restart, F3 pause, F1 help, F4 about, F5 sound, Esc.
Details: [`keyboard-simulator/app/README.md`](keyboard-simulator/app/README.md),
[`keyboard-simulator/core/README.md`](keyboard-simulator/core/README.md).

### Painter

PNG images and sprite sheets (PNG + JSON frames and animations): pencil, eraser, fill, line,
rectangles, picker, selection with copy/paste, frame and pivot tools, animation preview, onion
skin, palettes, undo/redo, and the PAINTER2 keyboard mode (Tab, arrows, Space).
Details: [`tools/painter/README.md`](tools/painter/README.md).

### Saves

Saves and settings are in `%APPDATA%\V Software\<game>`: `Soldier` (players, records,
settings), `Piton` (`piton.json`: level reached, high score, settings), `Keyboard Simulator`
(`settings.json`). The editors keep their window layout there too.

## Building

Requirements: Windows, Visual Studio 2022 with "Desktop development with C++", CMake 3.28 or
newer, Git, and an internet connection for the first configure (dependencies are fetched with
FetchContent at pinned versions: SDL 3.4.18, Dear ImGui 1.93.0, stb, nlohmann/json, doctest).

```
cmake --preset vs2022
cmake --build --preset release          # or: --preset debug
ctest --preset release                  # unit tests (doctest) + the editor round trip
```

Every program gets its own folder with its data and `SDL3.dll`:

```
build/vs2022/bin/Release/
├─ Soldier/              soldier.exe, soldier_editor.exe, SDL3.dll, data/
├─ Piton/                piton.exe, piton_editor.exe, SDL3.dll, data/
├─ KeyboardSimulator/    keyboard_simulator.exe, SDL3.dll, data/
├─ Painter/              painter.exe, SDL3.dll
└─ *_tests.exe, engine-demo.exe, SDL3.dll
```

Headless checks (no window, no sound card):

```sh
export SDL_VIDEO_DRIVER=offscreen SDL_AUDIO_DRIVER=dummy
build/vs2022/bin/Release/Soldier/soldier.exe --smoke                 # every program has --smoke
build/vs2022/bin/Release/Soldier/soldier.exe --verify-replays        # the 51 LISIY demos must win
build/vs2022/bin/Release/Soldier/soldier_editor.exe --roundtrip soldier/data/levels
build/vs2022/bin/Release/Piton/piton_editor.exe --roundtrip piton/data/levels
build/vs2022/bin/Release/Painter/painter.exe --roundtrip soldier/data/gfx "$TEMP/rt"
```

## Packaging

```
powershell -ExecutionPolicy Bypass -File tools/package.ps1
```

builds Release, runs `cmake --install build/vs2022 --config Release --prefix dist` and zips each
folder. Every folder is self-contained (the exes, `SDL3.dll`, the Visual C++ runtime DLLs,
`data/`, a `README.txt` with the controls):

```
dist/
├─ Soldier 2.0/           Soldier 2.0.zip
├─ Piton/                 Piton.zip
├─ Keyboard Simulator/    Keyboard Simulator.zip
└─ Painter/               Painter.zip
```

## Layout

```
remakes/
├─ README.md
├─ CMakeLists.txt, CMakePresets.json
├─ cmake/                        dependencies (FetchContent), output folders, install rules
├─ archive/                      originals, byte-for-byte (see archive/README.md)
│  ├─ soldier-1.0/  soldier-1.1-patch/  soldier-2.0/  piton/  diplom-2007/
│  └─ dosbox/                    DOSBox configs to run the originals
├─ engine/                       vs_engine (app, gfx, text, audio, input, ui, storage),
│                                vs_imgui, vs_editor (tile map editor base), tests
├─ shared/branding/              V Software Entertainment logo art (company intro)
├─ soldier/{core,game,editor,data,package}/
├─ piton/{core,game,editor,data,package}/
├─ keyboard-simulator/{core,app,data,package}/
├─ tools/
│  ├─ painter/                   graphics editor
│  ├─ engine-demo/               engine test program
│  ├─ import/                    converters of the original data (Python)
│  ├─ assets/                    generators of the art, sounds, music, fonts and icons (Python)
│  └─ package.ps1                Release build + dist/ + zips
└─ docs/                         design spec, file formats, asset notes, review of the originals
```

`core/` folders are the SDL-free logic libraries with their unit tests; `package/README.txt` is
the read-me shipped in `dist/`. The Python tools need the virtual environment described at the
top of `tools/assets/build_all.py` (`python -m venv .venv`, then `pip install pillow numpy scipy
soundfile`).

## Running the originals

See [`archive/dosbox/README.md`](archive/dosbox/README.md).

## Documentation

- Design: [`docs/specs/2026-10-22-remakes-design.md`](docs/specs/2026-10-22-remakes-design.md)
- Review of the originals: [`docs/originals-review.md`](docs/originals-review.md)
- File formats: [`docs/formats/`](docs/formats/)
- Assets and their sources: [`docs/assets.md`](docs/assets.md)
- Engine: [`engine/README.md`](engine/README.md)

Credits: V Software Entertainment — Program by Fedoseyenko Victor — Russia, Moscow, 2026.
