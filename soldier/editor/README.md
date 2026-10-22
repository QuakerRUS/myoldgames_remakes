# Soldier 2.0 Map Editor

`soldier_editor.exe` (CMake target `soldier-editor`): edits the JSON levels of the remake
(`soldier/data/levels/NN.json`, format in [`docs/formats/soldier-level.md`](../../docs/formats/soldier-level.md)).
Modelled on the original EDITOR: the field on the left, the palette on the right, a frame
cursor, Tab switches the panel. Built on `vs_editor` (engine) and `soldier_core`.

```sh
cmake --preset vs2022 -B build/soldier-editor
cmake --build build/soldier-editor --config Release --target soldier-editor
build/soldier-editor/bin/Release/Soldier/soldier_editor.exe [level.json]
```

## Controls

| Key / mouse | Action |
|---|---|
| Tab | switch the active panel: field ↔ palette (the active cursor is bright yellow) |
| Arrows | field: move the frame cursor (wraps around the edges); palette: move the selection (wraps) |
| Space / Enter | field: place the selected tile at the cursor; palette: pick the tile and return to the field |
| Backspace / Delete | reset the cell under the cursor to floor |
| Left mouse button | place (drag to paint; one undo step per stroke); on the palette: pick the tile |
| Right mouse button | erase to floor (drag to erase) |
| Wheel / middle button | zoom around the pointer / pan |
| Ctrl+Z, Ctrl+Y (Ctrl+Shift+Z) | undo, redo |
| Ctrl+N, Ctrl+O, Ctrl+S, Ctrl+Shift+S | new, open, save, save as (dialogs start in the levels folder) |
| F5 | test-play: saves to `%TEMP%\soldier_editor\testplay.json` and starts `soldier.exe --test-level <file>` from the editor's folder |
| Esc / closing the window | quit; with unsaved changes asks "Save the level?" Yes / No / Cancel (Y, N, Esc) |

Hovering a palette tile shows its name, code, file character and what it does; the selected
tile's hint is shown under the palette. The status bar names the tiles under the cursor and the
mouse.

## Tiles

The field is drawn with the game's sprites (`data/gfx/tiles.json`, `player.json`) and the same
composition: wall / dark-wall autotiling (outside the level counts as a plain wall: walls join
the map edge, dark walls do not, exactly as in the game), wall shadows, water edges, composed tiles (box on a
button, key in a door, player under a guillotine...), animated exit and water. Editor-only mark:
the fake wall (which looks exactly like a dark wall in the game) has a dashed outline and `?`.

Palette groups: **Basic** (floor, wall, exit, dark wall, fake wall), **Player start** (4 facings),
**Boxes, buttons and doors**, **Hazards and water** (guillotine, water, sunk box, box on sunk box),
**Keys and coloured doors** (per colour: key, door, key on button, key in door). Death tiles and
"player on ..." codes are not in the palette but are kept and drawn when a level contains them.

Placement rules: a player start keeps the ground it is placed on (button → player on button,
door → player in door, guillotine → player under guillotine). It *moves* the current start: the
start the game uses (the last start tile in row-major order) turns back into its ground; every
other player tile is kept as a decoration (levels 14 and 27 use them as pictures). If the new
tile is not the last player tile in row-major order, the status bar warns that the game will
start at the last one instead; nothing else is deleted.

## Properties and check

- **Level number** (1..999), **Width / Height** (1..100; type the size and press *Resize*: the
  top-left part is kept, new cells are floor), **Time limit** in original timer units
  (0 = no limit, 1 unit = 4 ticks = 2 s at normal speed).
- **Errors** (block F5): no player start; no exit.
- **Warnings** (F5 allowed): extra player tiles ("N extra player tiles are decorations; the start
  is the last one in scan order, as in the original" — shipped levels 14 and 27); no exit reachable from the start (flood fill through everything except walls,
  dark walls, water and decorations; doors and boxes count as passable); plain doors with no
  button (always open); coloured doors without (enough) keys of their colour; a level number that
  differs from a file named like a game level (`NN.json`): the game uses the file name's number
  for progress and records. Save As of a new level suggests `NN.json` from its level number.

## Files

Open accepts any level the game loads, and also levels without a start. Save writes exactly what
`soldier::level_to_json` writes (one row per line), atomically. The default folder is the source
tree's `soldier/data/levels` (development builds), else `data/levels` next to the exe.

## Command line

| Flag | |
|---|---|
| `<file>` or `--open <file>` | open a level at start |
| `--roundtrip <dir>` | no window: load + save every `*.json` of `<dir>` in memory and compare bytes; prints a summary, exit code 0 if all are identical |
| `--screenshot <png>` | render the UI once (after 12 frames) into a PNG and quit |
| `--smoke`, `--windowed` | engine flags (`--smoke` quits after one second) |

Headless checks:
```sh
SDL_VIDEO_DRIVER=offscreen SDL_AUDIO_DRIVER=dummy build/soldier-editor/bin/Release/Soldier/soldier_editor.exe --smoke
build/soldier-editor/bin/Release/Soldier/soldier_editor.exe --roundtrip soldier/data/levels
SDL_VIDEO_DRIVER=offscreen build/soldier-editor/bin/Release/Soldier/soldier_editor.exe --open soldier/data/levels/09.json --screenshot shot.png
ctest --test-dir build/soldier-editor -C Release -R soldier_editor_tests
```

## Source layout

| File | |
|---|---|
| `src/main.cpp` | command line; runs `vs::run_tile_editor` with the plugin |
| `src/soldier_plugin.*` | `vs::TileEditorPlugin`: palette groups, drawing hooks, rules, check, properties, panels, files, F5 |
| `src/tile_painter.*` | tile drawing from the game sprites with autotiling |
| `src/palette.*` | palette groups, tile names and hints, palette navigation (SDL-free) |
| `src/level_io.*` | document ↔ level JSON, round trip (SDL-free) |
| `src/level_check.*` | validation (SDL-free) |
| `src/placement.*` | placement and resize rules (SDL-free) |
| `tests/` | `soldier_editor_tests` (doctest) |

The editor is the engine's `vs::TileEditor` (window, field, palette, undo, files, prompts,
test-play) driven by `SoldierPlugin` through the plugin hooks: `draw_field_cell` (autotiling with
neighbour access) + `draw_cell_overlay` (guillotines, fake-wall marks) with one `overdraw_rows`,
palette groups (`TileInfo::group`) with a wrapping row-aware navigation, `check()` with errors and
warnings, `wrap_cursor`, `default_directory` (the levels folder), own status bar and panels.
The executable embeds `res/app.ico` (`res/app.rc`; generated by `tools/assets/gen_icons.py`).
