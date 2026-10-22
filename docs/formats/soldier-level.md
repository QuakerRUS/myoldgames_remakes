# Soldier level files

Two formats exist: the original binary `LEVELn.DAT` (Soldier 1.0/1.1, read-only, kept in
`archive/`) and the JSON files of the remake, `soldier/data/levels/NN.json` (`01.json` …
`51.json`), produced by `tools/import/soldier_levels.py`.

## Original: `LEVELn.DAT`

Little-endian, no padding.

| Offset | Size | Field |
|---|---|---|
| 0 | int16 | width (1..100) |
| 2 | int16 | height (1..100) |
| 4 | int16 | time limit in timer units; 0 = none (1.1 only — 1.0 had no "no limit") |
| 6 | int16 | level number (the game refused a file whose number differs from its name) |
| 8 | width × height bytes | tile codes, row by row (y outer, x inner) |

The 1.1 level set is `patch1_1.zip/LEVELS` plus levels 1, 24, 33 and 34 from 1.0 (absent from the
patch). The patch only zeroed the time limit of the other levels; the maps are identical to 1.0.

## Remake: `NN.json`

```json
{
  "number": 34,
  "width": 11,
  "height": 11,
  "time_limit": 15,
  "rows": [
    "###########",
    "#.........#",
    "..."
  ]
}
```

| Field | Meaning |
|---|---|
| `number` | level number 1..51 |
| `width`, `height` | 1..100 |
| `time_limit` | timer units (one unit = 4 ticks); `0` = no limit |
| `rows` | `height` strings of exactly `width` characters, top row first; one character per tile |

Unknown fields are ignored by the loader. The files are written with one row per line so that
diffs are readable.

### Player start

The start is the **last** player tile in row-major order among codes 3–6, 11–14, 16–19 and
49–52 (exactly as the original scan). Codes 57–60 are never a start. Any other player-looking
tile stays on the map as an inert decoration (levels 14 and 27 use them, together with death
tiles 7, 47 and 53, as pictures). A level without a start is rejected.

### Tile characters

The tile code is the byte of the original file. The character table is shared by
`tools/import/soldier_levels.py` (`TILE_CHARS`) and `soldier/core/src/tile.cpp`.

| Code | Char | Name (`soldier::tile_name`) | Notes |
|---:|:---:|---|---|
| 0 | `.` | floor | |
| 1 | `#` | wall | |
| 2 | `X` | exit | |
| 3–6 | `v` `^` `<` `>` | player_down/up/left/right | player on floor |
| 7 | `!` | death_gave_up | |
| 8 | `B` | box | |
| 9 | `_` | button | |
| 10 | `b` | box_on_button | |
| 11–14 | `1` `2` `3` `4` | player_on_button_down/up/left/right | |
| 15 | `D` | door | open while every button is pressed |
| 16–19 | `5` `6` `7` `8` | player_in_door_down/up/left/right | |
| 20 | `d` | box_in_door | |
| 21–24 | `r` `R` `a` `A` | red_key, red_door, red_key_on_button, red_key_in_door | |
| 25–28 | `g` `G` `h` `H` | green_… | |
| 29–32 | `u` `U` `i` `I` | blue_… | |
| 33–36 | `y` `Y` `j` `J` | yellow_… | |
| 37–40 | `w` `W` `l` `L` | white_… | |
| 41–44 | `k` `K` `m` `M` | black_… | |
| 45 | `%` | dark_wall | |
| 46 | `?` | fake_wall | looks like a dark wall |
| 47 | `@` | death_time_out | |
| 48 | `T` | guillotine | |
| 49–52 | `9` `0` `{` `}` | player_in_guillotine_down/up/left/right | |
| 53 | `$` | death_guillotine | |
| 54 | `~` | water | |
| 55 | `*` | death_drowned | |
| 56 | `=` | sunk_box | |
| 57–60 | `(` `)` `[` `]` | player_on_sunk_box_down/up/left/right | |
| 61 | `-` | death_gave_up_on_sunk_box | |
| 62 | `+` | death_time_out_on_sunk_box | |
| 63 | `&` | box_on_sunk_box | |

Per colour the four codes are, in order: key, door, key on a button, key in a (plain) door.
The behaviour of every tile is described in `soldier/core/README.md`.
