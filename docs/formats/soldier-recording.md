# Soldier recordings and players

## Remake: recording JSON

`soldier/data/records/<PLAYER>/NN.json` (`NN` = level number, two digits). The built-in demos
are `soldier/data/records/LISIY/01.json` … `51.json`, converted by
`tools/import/soldier_records.py`. The game writes the same format for new recordings
(`soldier::recording_to_json`).

```json
{
  "format": "soldier-recording",
  "version": 1,
  "level": 34,
  "player": "LISIY",
  "source": "REC034.DAT",
  "original": {"seed": 23737, "game_speed": 5000},
  "result": {"status": "won", "tick": 42},
  "commands": [
    [0, "up"], [2, "up"], [24, "cw"], [24, "use"], [26, "up"]
  ]
}
```

| Field | Required | Meaning |
|---|---|---|
| `format` | yes | always `"soldier-recording"` |
| `version` | yes | `1` |
| `level` | yes | level number |
| `player` | yes | player name; a record in a player's save folder counts towards his progress only if this is his name (see `soldier/core/README.md`, "Progress proven by records") |
| `commands` | yes | `[tick, command]` pairs; ticks are integers ≥ 0 and never decrease |
| `result` | no | how the recorded game ended: `{"status": "won" \| "dead" \| "playing", "tick": N, "cause": …}`; `cause` (only for `dead`) is `"gave_up"`, `"time_out"`, `"guillotine"` or `"drowned"` |
| `source` | no | informational: the file it was converted from |
| `original` | no | informational, imported files only: the record's random seed and the GAME SPEED (`gs&`) it was recorded at. Ignored by the loader |

Unknown fields are ignored.

Command names: `"down"`, `"up"`, `"left"`, `"right"` (arrow keys: step / push / bump),
`"cw"` (`.` `>`), `"ccw"` (`,` `<`), `"next"` (Tab), `"use"` (Space, Enter), `"giveup"` (Esc).

### Semantics

A tick is one "part" of the original's game second (4 ticks per second, see
`soldier/core/README.md`). `[t, c]` means: command `c` was **executed** during tick `t`, after the
commands listed before it. A recording is the `Game::history()` of a game. Playback
(`soldier::Replayer`) applies, at the start of the game and after every `Game::tick()`, all
commands whose tick equals the current tick, in file order. Because the history only contains
executed commands, a recorded move is never inside the move cooldown, so playback is exact;
`run_replay` reports `in_sync = false` if that is ever violated.

## Original: `RECORDS/REC<slot><level>.DAT`

Random-access file of 4-byte records (`OPEN … FOR RANDOM … LEN = 4`), little-endian.
The name is `"rec"` + slot digit + level number without padding: `REC023.DAT` = slot 0 level 23,
`REC110.DAT` = slot 1 level 10. Slot 0 is LISIY, slot 1 TARAS (not imported).

| Record | Content |
|---|---|
| #1 | int32 random seed (unused by the rules) |
| #2 | int32 `gs&`: GAME SPEED at recording time = loop iterations per part |
| #3k, #3k+1, #3k+2 (k ≥ 1) | int32 `numsec&` (part counter, from 0), int32 `f&` (iteration 1..gs within the part), 4-byte `INKEY$` value |

The key field holds `INKEY$` left-justified and space-padded: a one-character key (`,` `<` `.`
`>` space, Enter = CHR$(13), Tab = CHR$(9), Esc = CHR$(27)) is followed by a space; an extended
key is CHR$(0) + scan code (72 up, 80 down, 75 left, 77 right). Only the first two bytes are used.
The original replay (GAME1.BAS lines 20–315, subroutine 1930) executes a key in the first loop
iteration of part `numsec&` whose `f/gs` ≥ the recorded `f&/gs&`, at most one key per iteration,
without the move cooldown.

### Conversion

`tick = numsec&` of the iteration in which the original replay executes the key (equal to the
recorded `numsec&` for every LISIY record). The importer runs an exact transliteration of the
1.1 replay loop on the 1.1 level to verify this and stores its outcome as `result`. All 51 LISIY
records end with `won`; none moves inside the 2-tick cooldown, none depends on the push-right
bug, none leaves the level area. The records of levels 34–36 contain one-character keys (turns
and Space), which the 1.1 recorder could not write (an earlier build must have been used); they
replay correctly.

## Original saves: `GAMESAVn.DAT`

Random-access file of 2-byte records. Records #1–#4 (bytes 0–7): the player name, 8 characters,
space-padded. Record #66 (bytes 130–131): int16, the level reached (the highest level the player
may start; 52 means all 51 levels are completed). Records #5–#106 also hold anti-tamper noise:
record `f` must equal record `213 - f` minus 1, otherwise the original menu hung the computer
(not reproduced in the remake).

## Remake: `soldier/data/players.json`

```json
{
  "players": [
    {"name": "LISIY", "level_reached": 52, "builtin": true}
  ]
}
```

`level_reached` has the meaning of save record #66. `builtin: true` marks the shipped demo
player (imported from `GAMESAV0.DAT`). Where the game stores the profiles of new players is up
to the game (the engine's storage paths).
