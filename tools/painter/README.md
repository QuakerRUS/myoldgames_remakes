# Painter

The shared graphics editor of Soldier and Piton (`painter.exe`, window "Painter — V Software"),
in the spirit of PAINTER2/EDITOR from the 2000–2001 QBasic games: a pixel grid with a frame
cursor, a palette, Tab between them, the keyboard alone is enough to draw.

It opens and saves **PNG images** and **sprite sheets** (a PNG plus the engine's JSON next to it,
see `engine/README.md`, "Sprite sheet JSON").

## Running

```
painter [file.png | sheet.json]
painter --open soldier/data/gfx/player.json [--frame walk_d_1]
```

Command-line checks (no visible window needed for the last three:
`SDL_VIDEO_DRIVER=offscreen SDL_AUDIO_DRIVER=dummy`):

| Flag | What it does |
|---|---|
| `--roundtrip <dir> <outdir>` | Opens every `*.json` sheet and every other `*.png` of `<dir>` exactly as the editor does, saves it unchanged into `<outdir>`, reloads and compares pixels and JSON semantics. Exit code 0 = all identical. Does not open a window. |
| `--smoke` | Starts, runs one second, quits (exit code 0). |
| `--screenshot <out.png> --open <file> [--frame <name>]` | Renders a few frames, saves one as PNG, quits. |
| `--selftest <scratch dir> --open <sheet.json>` | Drives every kind of edit through the editor (keyboard painting, all tools, selection, frames, animations, canvas size), checks that undoing everything restores the file exactly, redo restores the edits, and save + reload keeps them. |

```sh
build/painter/bin/Release/Painter/painter.exe --roundtrip soldier/data/gfx "$TMP/rt/soldier"
build/painter/bin/Release/Painter/painter.exe --roundtrip piton/data/gfx "$TMP/rt/piton"
```

## Layout

- **Tool strip** (left): the tools, the foreground/background colours (click to swap).
- **Document panel**: the frame list (add, duplicate, delete, reorder), the selected frame's
  name, position, size and pivot; the animation list, the selected animation's name, loop flag
  and entries (frame + milliseconds, reorder, remove, append the selected frame, set all
  durations); a live check that the sheet will load in the game.
- **Canvas**: checkerboard under transparent pixels, integer zoom 1–64×, pixel grid (from 6×),
  optional tile grid (size and gap, e.g. 32 + 1 for Soldier, 24 + 1 for Piton), frame outlines
  with the selected frame and its pivot, onion skin of the previous frame in Frame view.
  **Sheet** view shows the whole image; **Frame** view (F, or double-click a frame) shows the
  selected frame alone and confines painting to it.
- **Colour panel**: RGBA picker and hex for the foreground (or background) colour, 16 recent
  colours, the palette (built from the image's colours, sorted; or loaded), and the animation
  preview with play/pause, step and scale.
- **Status bar**: tool, keyboard focus, cursor, pixel under the pointer with its colour, image
  size, selection, zoom, last message.

## Controls

### Mouse

| Input | Action |
|---|---|
| LMB / RMB | Use the tool with the foreground / background colour |
| Alt + click | Colour picker (LMB → foreground, RMB → background) |
| Wheel | Zoom around the pointer (1, 2, 3, 4, 5, 6, 8 … 64×) |
| Middle drag, Space + LMB drag | Pan |
| Palette: LMB / RMB | Take as foreground / background |

### Keyboard mode (as in PAINTER2)

| Key | Canvas focus | Palette focus |
|---|---|---|
| Tab | Switch to the palette | Switch to the canvas |
| Arrows (Shift: 8 px) | Move the frame cursor (or a pasted selection) | Move the palette selection |
| Space, Enter | Paint the foreground colour at the cursor (Enter places a pasted selection) | Take the colour as foreground, back to the canvas |
| Backspace | Paint the background colour (transparent by default) | Take the colour as background |
| Delete | Erase the pixel or the selection to transparent | |
| F1 | Flood fill from the cursor | |
| Esc | Cancel a paste → deselect → "Save changes?" Yes/No/Cancel, then quit | |

Space paints when it is released, so that Space + drag can pan without painting.

### Tools and commands

| Key | Action |
|---|---|
| B, E, G, L | Pencil, eraser (transparent), fill (4-way, exact colour), line |
| R, Shift+R | Rectangle outline, filled rectangle |
| I | Colour picker |
| M | Select: drag a rectangle; drag inside it to move the pixels; click outside to place |
| T | Frame rectangle (sheets): click selects a frame, drag sets its rectangle (snaps to the tile grid) |
| Y | Pivot (sheets): click sets the selected frame's pivot |
| X | Swap foreground and background |
| Shift+H, Shift+V | Flip the selection, else the frame (Frame view), else the image |
| Ctrl+C, Ctrl+X, Ctrl+V | Copy, cut, paste (as a floating selection; transparent pixels do not overwrite) |
| Ctrl+A, Ctrl+D | Select all (the frame in Frame view), deselect |
| Ctrl+Z, Ctrl+Y / Ctrl+Shift+Z | Undo, redo (every pixel and sheet edit; one stroke = one step) |
| Ctrl+N, Ctrl+O, Ctrl+S, Ctrl+Shift+S | New, open, save, save as |
| F, [ , ] | Frame view on/off, previous / next frame |
| O, P | Onion skin, play/pause the preview |
| +, -, Home | Zoom in, zoom out, fit |

While a selection exists, painting tools only change pixels inside it.

## Files

- Opening a `.json` loads the sheet and the PNG named by its `"image"`; any other file is
  loaded as an image. **Image → Make sprite sheet** turns a PNG into a sheet.
- Saving a sheet writes the PNG and then the JSON (both atomically) in the engine's canonical
  form (`vs::sprite_sheet_to_json_text`): frames in list order, animations with per-frame
  `{"frame", "ms"}` entries. A `"grid"` block is expanded into explicit frames; other unknown
  top-level keys are kept. **Save As** `name.json` writes `name.png` beside it (and `name.png`
  writes `name.json`). If that companion file already exists and is not part of the open
  document, Painter asks "Replace ...?" before writing anything.
- Undo keeps up to 400 steps and about 512 MB of stored pixels; the oldest steps are dropped
  beyond that. An operation that runs out of memory is cancelled with a message instead of
  closing the program. Shortcuts are ignored while a mouse stroke is in progress.
- Saving without edits reproduces identical pixels and an equivalent JSON (`--roundtrip`).
- Palettes: **Load/Save** `.hex` (one `RRGGBB` or `RRGGBBAA` per line, Lospec style) or `.gpl`
  (GIMP; no alpha, the transparent entry is skipped).
- Closing the window, Esc, New and Open ask "Save?" when there are unsaved changes; the title
  shows the file name and `*`.

## Source

| File | Contents |
|---|---|
| `src/main.cpp` | arguments, App setup |
| `src/painter_scene.*` | Scene hosting ImGui + Painter; screenshot, self-test hooks |
| `src/painter.hpp`, `src/painter.cpp` | the editor class; per-frame layout and texture sync |
| `src/painter_file.cpp` | new/open/save, unsaved-changes prompt, title |
| `src/painter_edit.cpp` | undo plumbing (`begin_edit`/`end_edit`, sheet edits), view rect, clip |
| `src/painter_selection.cpp` | selection, clipboard, floating pixels, flips |
| `src/painter_tools.cpp` | mouse tools, keyboard painting |
| `src/painter_canvas.cpp` | canvas drawing, zoom, toolbar |
| `src/painter_keys.cpp` | keyboard shortcuts and keyboard mode |
| `src/painter_menu.cpp` | menu bar, tool strip, status bar, controls popup |
| `src/painter_frames.cpp`, `src/painter_animations.cpp` | frame and animation panels, preview |
| `src/painter_colors.cpp`, `src/painter_dialogs.cpp` | colour/palette panel; New and Canvas size popups |
| `src/painter_selftest.cpp` | `--selftest` |
| `src/tools.*`, `src/ui_util.*` | tool names and icons; ImGui helpers and style |
| `src/document.*`, `src/path_util.hpp`, `src/sheet_json.cpp`, `src/sheet_ops.*` | SDL-free document (save targets, companion check, `lower_ext`), sheet JSON (extra keys, equivalence, validation), frame/animation operations |
| `src/raster.*`, `src/palette.*`, `src/commands.*`, `src/roundtrip.*` | SDL-free pixel algorithms, palettes, undo commands, `--roundtrip` |
| `tests/` | doctest unit tests (`painter_tests`): raster, palettes, sheets |

From the engine: `vs::GridView` with `integer_zoom` (wheel and +/- steps), `vs::attach_parent_console`
for the command-line checks, `AppConfig::screenshot_path` for `--screenshot` (frame 12),
`ImGui::InputText(label, std::string*)` from `imgui_stdlib`. The executable embeds `res/app.ico`
(`res/app.rc`; generated by `tools/assets/gen_icons.py`).
