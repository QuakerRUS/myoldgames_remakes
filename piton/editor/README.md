# Piton Map Editor

`piton_editor.exe` (CMake target `piton-editor`) edits Piton levels: the fixed 38×23 field
of bricks and grass, the level name, and the snake's start cell and direction. It saves the
`piton-level` JSON format ([docs/formats/piton-level.md](../../docs/formats/piton-level.md))
byte for byte as `piton::Level::toJson()` writes it.

The UI follows the Soldier 2.0 EDITOR model used by `vs_editor`: the field on the left, the
palette, properties, tools and level check on the right, a status bar at the bottom.

```
cmake --preset vs2022 -B build/piton-editor
cmake --build build/piton-editor --config Debug --target piton-editor
build/piton-editor/bin/Debug/Piton/piton_editor.exe [level.json]
```

Open/Save dialogs start in `piton/data/levels` (the source folder during development,
otherwise `data/levels` next to the exe). A new level is an empty field with the start in
the middle (head at 19,11) facing right.

## Palette

| Key | Tile | Placing it |
|---|---|---|
| 1 | Brick | sets the cell to a brick |
| 2 | Empty | sets the cell to grass |
| 3–6 | Snake start Up / Right / Down / Left | moves the start head to the cell with that direction; the bricks under the three start cells are removed |

The field is drawn with the game sprites: grass variants, brick autotiles with shading, and
the 3-segment start snake (head, body, tail). White dots mark the 6 cells the snake needs
free ahead of its head (red when blocked).

## Controls

| Key / mouse | Action |
|---|---|
| Tab | switch the active panel: field ↔ palette |
| Arrows | move the frame cursor (field) or the palette selection |
| Space / Enter | place the selected tile at the cursor (in the palette: pick it and return to the field) |
| Backspace / Delete | erase (make the cell empty) |
| 1–6 | select a palette entry |
| R | rotate the start direction clockwise |
| H / V | toggle horizontal / vertical symmetry painting (both = 4-way) |
| G | grid lines on/off |
| Home | fit the field to the window |
| Ctrl+Z, Ctrl+Y / Ctrl+Shift+Z | undo, redo |
| Ctrl+N / Ctrl+O / Ctrl+S / Ctrl+Shift+S | new, open, save, save as |
| F5 | test-play: saves to `%TEMP%\piton_editor_testplay.json` and starts `piton.exe --test-level <file>` from the editor's folder |
| Esc / closing the window | quit; with unsaved changes asks "Save?" Yes / No / Cancel (Y/Enter, N, Esc) |
| Left mouse | place (dragging paints bricks/empty, one undo step per stroke; the start is placed on click) |
| Right mouse | erase |
| Wheel / middle button | zoom around the pointer / pan |

## Properties and tools

- **Name**: the level name shown in the game (committed when the field loses focus or on Enter).
- **Start direction**: Up / Right / Down / Left; **Head x,y** moves the start (Enter applies).
- **Tools** (also in the Tools menu): Clear field (start kept), Fill border (outer ring of
  bricks), Flip horizontally / vertically (the whole level including the start), Mirror left
  half → right, Mirror top half → bottom (make the level symmetric).
- **Symmetry painting**: Off / Horizontal / Vertical / Both — every painted cell is mirrored
  across the field's centre lines (shown as blue axes). The start is never mirrored.

Every edit is one undo step.

## Level check

`piton::checkLevel` runs live; the Check panel shows free cells, regions, the longest dead
end and the list of problems: hard errors in red (a start cell not free, no free cell for a
rabbit, more than one region — the level cannot be played or completed), warnings in yellow
(room ahead of the start, too few free cells, long dead ends). Saving is always allowed (a
popup lists the errors); F5 refuses a level with errors (the game's `--test-level` refuses it
too) and test-plays one with warnings only.

The start snake always lies inside the field: placing (or rotating, or typing) a start whose
body would stick out moves the head inwards and says so in the status bar. Bricks are never
painted on the start cells.

## Command line

| Arguments | What |
|---|---|
| `piton_editor [level.json]` | open a level |
| `--roundtrip <dir>` | self-check: every `*.json` in `<dir>` is loaded and saved through the editor's document model; the bytes must equal the original. Prints one line per file; exit code 0 when all match |
| `--screenshot <file.png>` | render 30 frames, save the window as PNG, quit (visual check) |
| `--smoke` | start, run one second, exit 0 |

Run without visible windows:

```sh
SDL_VIDEO_DRIVER=offscreen SDL_AUDIO_DRIVER=dummy build/piton-editor/bin/Debug/Piton/piton_editor.exe --smoke
build/piton-editor/bin/Debug/Piton/piton_editor.exe --roundtrip piton/data/levels
```

## Tests

`piton_editor_tests` (doctest): new document, symmetry painting, start placement, the tools,
document ↔ level conversion and the byte-identical round trip of all shipped levels.
CTest also runs `piton_editor_roundtrip` (the exe's `--roundtrip` over `piton/data/levels`).

```
ctest --test-dir build/piton-editor -C Debug -R piton_editor
```

## Source

| File | What |
|---|---|
| `src/level_doc.*` | SDL-free: `piton::Level` ↔ `vs::TileDocument`, symmetry, tools, load/save, round trip |
| `src/piton_editor.*` | `PitonPlugin`, the `vs::TileEditorPlugin` (field rendering, start snake, symmetry, palette, properties, tools, check, files, test-play) |
| `src/main.cpp` | program entry, `--roundtrip`; runs `vs::run_tile_editor` (`--screenshot` via `vs::parse_tile_editor_args`) |

The editor is the engine's `vs::TileEditor` (window, field, palette, undo, files, prompts,
test-play) driven by `PitonPlugin` through the plugin hooks: `draw_field_cell` (brick autotiles,
grass shading), `draw_field_overlay` (start snake, free cells ahead, symmetry axes),
`draw_hover`/`draw_cursor` (snake preview, mirrored cursors), `place_document` (the start is a
document change, placed on click only), `check()` + `errors_block_test_play() == false` +
`report_errors_on_save()`, extra menu, panels, keys and status bar. The executable embeds
`res/app.ico` (`res/app.rc`; generated by `tools/assets/gen_icons.py`).
