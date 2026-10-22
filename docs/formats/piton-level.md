# Piton level format (`piton-level`, version 1)

Piton levels live in `piton/data/levels/01.json` … `10.json` (two-digit level number).
They are UTF-8 JSON files read and written by `piton::Level` (`piton/core`); the map
editor saves the same format.

## Example

```json
{
  "format": "piton-level",
  "version": 1,
  "name": "Twin Walls",
  "width": 38,
  "height": 23,
  "start": {
    "x": 16,
    "y": 19,
    "dir": "right"
  },
  "rows": [
    "......................................",
    "...",
    ".......########################......."
  ]
}
```

(`rows` is shortened here; a real file has 23 rows.)

## Fields

| Field | Type | Meaning |
|---|---|---|
| `format` | string | Always `"piton-level"`. |
| `version` | integer | Always `1`. |
| `name` | string | Level name shown in the game. Optional, may be empty. |
| `width`, `height` | integer | Always `38` and `23`. |
| `start.x`, `start.y` | integer | The snake head's start cell, `0 ≤ x < 38`, `0 ≤ y < 23`; (0,0) is the top-left cell. |
| `start.dir` | string | Start direction: `"up"`, `"right"`, `"down"` or `"left"`. |
| `rows` | array of 23 strings | The field from top to bottom, each string 38 characters, left to right. |

Row characters:

| Char | Cell |
|---|---|
| `.` | empty (grass) |
| `#` | brick |

The field is surrounded by an implicit border: leaving the 38×23 grid kills the snake like
a brick. The snake starts with 3 segments: the head at `start` and two more cells straight
behind it (opposite to `start.dir`). Rabbits are not stored; the game places them.

Unknown fields are ignored by the reader; the writer emits exactly the fields above, in this
order, with 2-space indentation and LF line endings.

## Validity rules

`piton::checkLevel()` enforces these; the tests run them on all shipped levels and the
editor shows the problems:

1. The three start cells are inside the field and empty.
2. At least 6 empty cells straight ahead of the head.
3. All empty cells form one 4-connected region (every cell is reachable).
4. At least 159 empty cells (3 × the full-grown snake of 3 + 50 segments).
5. No 1-cell-wide dead-end corridor longer than 3 cells (a long snake that enters one
   cannot turn around). It is measured by repeatedly removing empty cells with at most
   one empty neighbour; the number of rounds is the longest dead end.

Solvability is further checked by the `piton_core_tests` simulation: the demo AI eats all
50 rabbits on every level for several seeds.
