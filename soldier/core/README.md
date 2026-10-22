# soldier_core

SDL-free game logic of Soldier 2.0: the level model, the rules (an exact port of `GAME1.BAS`
version 1.1 onto a discrete tick clock), events for the presentation layer, recordings and
their playback. Static library `soldier_core`, namespace `soldier`, C++20, no dependencies
in its public headers (nlohmann/json is used privately).

```cmake
target_link_libraries(my_target PRIVATE soldier_core)
```

| Header | Contents |
|---|---|
| `soldier/tile.hpp` | `Tile` (codes 0–63), `Dir`, `KeyColor`, `Ground`, tile helpers and names |
| `soldier/level.hpp` | `Level`, `Point`, `FormatError`, level JSON load/save |
| `soldier/command.hpp` | `Command`, `CommandType` |
| `soldier/event.hpp` | `Event`, `EventType`, `DeathCause` |
| `soldier/game.hpp` | `Game`, `Status`, `RecordedCommand`, constants |
| `soldier/recording.hpp` | `Recording`, JSON load/save, `Replayer`, `run_replay` |
| `soldier/progress.hpp` | `record_proves_win`, `verified_level_reached`: progress proven by records |

File formats: [`docs/formats/soldier-level.md`](../../docs/formats/soldier-level.md),
[`docs/formats/soldier-recording.md`](../../docs/formats/soldier-recording.md).
Data: `soldier/data/levels/NN.json`, `soldier/data/records/LISIY/NN.json`,
`soldier/data/players.json`.

Tests: `cmake --preset vs2022 -B build/soldier-core -DVS_LOGIC_ONLY=ON`,
`cmake --build build/soldier-core --config Release --target soldier_core_tests`,
`ctest --test-dir build/soldier-core -C Release`.

## Typical use

```cpp
#include "soldier/game.hpp"
#include "soldier/recording.hpp"

soldier::Level level = soldier::load_level_file("soldier/data/levels/01.json");  // throws FormatError
soldier::Game game(level);
double acc = 0;
for (;;) {                                   // frame loop of the game
    for (auto key : keys_pressed_this_frame) // map keys to commands
        game.apply(command_for(key));        // executed immediately (or buffered, see cooldown)
    acc += frame_seconds;
    while (acc >= tick_seconds && game.status() == soldier::Status::Playing) {
        acc -= tick_seconds;                 // 0.5 s at speed x1 (x0.5 .. x2 only change this)
        game.tick();
    }
    for (const soldier::Event& e : game.take_events()) animate_and_play_sound(e);
    draw(game);                              // cells, player, inventory, timer, part()
    if (game.status() != soldier::Status::Playing) break;
}
auto rec = soldier::make_recording(game, level.number, "PLAYER");  // save with recording_to_json
```

Playback:

```cpp
soldier::Replayer r(level, soldier::load_recording_file("soldier/data/records/LISIY/01.json"));
// every tick period:
r.step();                       // = r.game().tick(), then the recorded commands of the new tick
for (auto& e : r.game().take_events()) animate_and_play_sound(e);
draw(r.game());                 // stop when r.finished()
```

The original replay ended the demo (as a death by giving up) when a key was pressed; that is a
presentation decision now.

## Time model

| Original (`GAME1.BAS`) | Remake |
|---|---|
| a game second = 4 *parts* (`numpartsec` 1..4); each part is `gs&` loop iterations | a second = 4 **ticks**; `Game::part()` = `tick_count() % 4 + 1` |
| `numsec&` = parts since the start, from 0 | `Game::tick_count()`, from 0 |
| keys polled every iteration and handled in that iteration | `Game::apply()` executes the command at once, inside the current tick, in call order |
| a step sets `p& = gs& * 2`; the next step is accepted 2 parts later | a step at tick *t* allows the next step at tick *t + 2* (`move_ready_tick()`) |
| a step key pressed during the cooldown stays in `i$` (one-key buffer, the last key wins) and is executed as soon as `p&` reaches 0 | `apply(Move)` during the cooldown buffers it (`buffered_move()`), the last one wins; it is executed at the start of tick *t + 2*, inside `tick()` |
| any other key (turn, Space/Enter, Tab, but also every key the game ignores) overwrote `i$` | any non-move command discards the buffered move; `discard_buffered_move()` does the same for keys that are not commands (the game calls it for every other key press, including unmapped keys, P and Z; Esc is a command). A discarded move is never executed, so it is not in `history()` and recordings stay exact |
| guillotine closed during part 4 | closed during ticks with `part() == 4` |
| player under a guillotine when part 4 begins: killed | checked in `tick()` when the new tick has `part() == 4` |
| after part 4: if `time = 0` → death, else (if a limit exists) `time = time - 1` (subroutine 1700) | in `tick()` when the new tick has `part() == 1`, same order |

Sub-tick timing therefore maps as: **a command issued during part *k* is applied during tick *k*,
in order**; only the order of commands within a tick matters, because no rule depends on the
position inside a part. The original records store, for every executed key, its `numsec&`;
that is directly the tick (see the recording format). The only rounding is that the original
accepted the next step 2 parts later *at the same iteration*, the remake at the start of that
tick — invisible in every original record.

The timer: `timer()` starts at `Level::time_limit` (or −1 when there is no limit), decreases by 1
at ticks 4, 8, 12, …, and the player dies at the first second boundary at which it is already 0:
at tick `4 × (time_limit + 1)`. With 500 ms ticks a timer unit lasts 2 s.

## `Game`

```cpp
explicit Game(Level level);      // throws FormatError if the level has no start
void apply(Command c);           // ignored unless status() == Playing
void tick();                     // ignored unless status() == Playing
void discard_buffered_move();    // a key that is not a command was pressed
std::vector<Event> take_events();
```

`Command` (`command.hpp`): `Command::move(Dir)`, `turn_cw()`, `turn_ccw()`, `next_item()`,
`use_item()`, `give_up()`. `command_name` / `command_from_name` give the recording names.

State accessors (all `const`):

| Accessor | Meaning |
|---|---|
| `level()`, `cell(Point)`, `width()`, `height()` | current tiles (they change: the player tile, boxes, keys, death tiles) |
| `player()`, `facing()`, `ground()` | position, facing `Dir`, what the player stands on (`Floor`, `Button`, `Door`, `Guillotine`, `SunkBox`); the player's cell holds `player_tile(ground(), facing())` while playing |
| `unpressed_buttons()`, `doors_open()` | the original `onkey`; plain doors (15) are open iff it is 0 |
| `inventory()`, `inventory_cursor()` | keys (`KeyColor`) in pick-up order, at most `kInventorySize` = 25; cursor 0-based, 0 when empty |
| `tick_count()`, `part()`, `guillotines_closed()` | clock |
| `timer()` | timer units left, −1 = no limit |
| `move_ready_tick()`, `can_move_now()`, `buffered_move()` | move cooldown |
| `status()`, `death_cause()`, `death_tile()` | `Playing` / `Won` / `Dead`; cause `GaveUp`, `TimeOut`, `Guillotine`, `Drowned`; the death tile (7, 47, 53, 55, 61, 62) is also written into the player's cell |
| `history()` | executed commands with their ticks (= the recording) |

Coordinates are 0-based (`x` right, `y` down); `Level::cells[y * width + x]`. Cells outside the
level behave as walls (see "Differences from the original", item 5).

## Rules

`Move(d)`: let *T* be the cell in front (player + d), *B* the cell beyond it. "Step" = the player
leaves its cell (the ground tile is restored there; leaving a button releases it), enters *T*
facing *d*, and the 2-tick cooldown starts. "Bump" = the player turns to face *d* (on any ground)
without moving and without cooldown.

| *T* | Effect of `Move(d)` |
|---|---|
| floor 0 | step |
| fake wall 46 | step (`FakeWallEntered`); the cell is floor after the player leaves |
| wall 1, outside the level | bump |
| dark wall 45 | becomes wall 1 (`WallRevealed`), bump |
| exit 2 | the player leaves its cell and stands on the exit; `Won` |
| button 9 | step onto the button, pressing it |
| door 15 | step into the door if `doors_open()`, else bump |
| box 8 / box on sunk box 63 | if *B* is floor, button, open door, water or sunk box: the box moves to *B* (floor → 8, button → 10 pressing it, door → 20, water → 56 `BoxSank`, sunk box → 63) and the player steps onto *T*'s ground (floor / sunk box); otherwise bump |
| box on button 10 / box in door 20 | same, but *B* must be floor, button or open door (no water, no sunk box); the player steps onto the button / into the door (a box in a closed door can still be pushed out) |
| coloured key 21/25/… | if the inventory is full (25): bump; else the key is appended (`KeyPicked`) and the player steps; key on button 23/27/…: the player then stands on the (still pressed) button; key in door 24/28/…: the player stands in the door, even when it is closed |
| coloured door 22/26/… | if the inventory has a key of that colour, the first such key is consumed, the door becomes floor (`ColorDoorUnlocked`) and the player steps in; else bump |
| guillotine 48 | bump during part 4, else step under it; the player dies if he is still there when part 4 begins |
| water 54 | the player steps in and drowns; the cell becomes 55 |
| sunk box 56 | step |
| decorations (other player tiles, death tiles 7/47/53/55/61/62) | nothing (no turn, no cooldown); a `Bumped` event is emitted |

Other commands (immediate, no cooldown):

- `TurnCW` / `TurnCCW`: turn clockwise (down → left → up → right → down) / counter-clockwise.
- `NextItem`: cursor to the next item, wrapping; nothing when empty.
- `UseItem`: with the selected key, on the cell in front: floor → the key lies there; button →
  key on button (pressed); plain door while `doors_open()` → key in door; door of the key's
  colour → floor. The key leaves the inventory and the cursor returns to 0. Otherwise nothing.
- `GiveUp`: death (`GaveUp`); tile 7, 53 under a guillotine, 61 on a sunk box.

Time-out death tiles: 47, 53 under a guillotine, 62 on a sunk box.

## Events

`Event` fields: `type`, `at`, `to`, `dir`, `color`, `cause`, `tile`, `value`; only the ones listed
are meaningful. Events of one `apply()`/`tick()` come in the order the rules ran.

| Type | Fields | When |
|---|---|---|
| `Moved` | `at` (from), `to`, `dir` | any step, including onto the exit and into water |
| `Turned` | `dir` (new facing) | turn command, or a bump that changed the facing |
| `Bumped` | `at` (blocked cell), `dir` | a blocked move |
| `BoxPushed` | `at`, `to`, `dir` | before the player's `Moved` |
| `BoxSank` | `at` | after `BoxPushed` into water |
| `ButtonPressed` / `ButtonReleased` | `at` | by the player, a box or a dropped key / the player left it |
| `DoorsOpened` / `DoorsClosed` | — | once per command, when `doors_open()` changed |
| `KeyPicked` | `at`, `color` | |
| `KeyDropped` | `at`, `color`, `tile` | `UseItem` placed the key (`tile` = 21.., 23.. or 24..) |
| `ColorDoorUnlocked` | `at`, `color` | by walking into it or by `UseItem` |
| `WallRevealed` | `at` | dark wall bumped |
| `FakeWallEntered` | `at` | |
| `ItemSelected` | `value` (cursor) | `NextItem` (other inventory changes are implied by `KeyPicked`, `KeyDropped`, `ColorDoorUnlocked`) |
| `GuillotinesClosed` / `GuillotinesOpened` | — | every tick that starts part 4 / part 1 (also on levels without guillotines) |
| `TimerChanged` | `value` | the timer decreased |
| `Died` | `at`, `cause`, `tile` | |
| `Won` | `at` | |

`event_name`, `death_cause_name`, `tile_name`, `key_color_name`, `dir_name` return stable
lower-case identifiers (for logs, sound tables, etc.).

## Recordings

```cpp
struct RecordedCommand { int tick; Command command; };
struct RecordedResult  { Status status; int tick; std::optional<DeathCause> cause; };
struct Recording { int level; std::string player; std::vector<RecordedCommand> commands;
                   std::optional<RecordedResult> result; std::string source; };
Recording parse_recording_json(std::string_view);   Recording load_recording_file(const std::string&);
std::string recording_to_json(const Recording&);
Recording make_recording(const Game&, int level_number, std::string player);  // history + result

class Replayer { Replayer(Level, Recording); Game& game(); void step();
                 bool finished() const; bool commands_left() const; std::size_t applied() const; };
ReplayOutcome run_replay(const Level&, const Recording&, int extra_ticks = 4000);
// ReplayOutcome { Status status; DeathCause cause; int tick; std::size_t unused_commands; bool in_sync; }
```

Level helpers (`level.hpp`): `parse_level_json`, `load_level_file`, `level_to_json`,
`read_text_file`, `Level::start()`, `Level::at(Point)`, `Level::in_bounds(Point)`.
All loaders throw `soldier::FormatError` (a `std::runtime_error`).

## Progress proven by records

The game does not trust `level_reached` in `save.json` (the original hung the computer when its
save looked tampered with; the remake quietly corrects it instead):

```cpp
bool record_proves_win(const Level&, const Recording&, std::string_view player);
int verified_level_reached(int level_count, std::string_view player,
                           std::function<std::optional<Level>(int)> level_for,
                           std::function<std::optional<Recording>(int)> record_for);
int verified_level_reached(const std::filesystem::path& slot_dir, const std::filesystem::path& levels_dir,
                           int level_count, std::string_view player);   // NN.json files
```

A record proves level *n* for a player if its `level` is *n*, its `player` is that player's name,
its result is `won`, and `run_replay` on the shipped level ends in a win at exactly the recorded
result tick. The verified progress is 1 + the largest *k* such that levels 1..*k* are all proven
(52 = all 51 completed). Every win writes a record, so honest play always verifies; the
built-in LISIY is verified with his shipped records. This is a consistency check, not
cryptography: a player who copies another player's records and edits their `player` field
gets that progress (the moves are a genuine solution anyway).

## Differences from the original (bugs fixed)

None of these changes affects any of the 51 LISIY demos (verified by the importer, which replays
each record with the unmodified original rules, and by the tests, which replay them with the
remake's rules and compare the final tick).

1. Pushing a box (8 or 63) to the right checked the cell two to the left of the player for a
   sunk box (`ml(absx - 2, absy)` in subroutines 2300 and 5600): such a push onto a sunk box was
   refused, and a box could vanish into a wall. Fixed.
2. Tab (`NextItem`) was unreachable in 1.1 (subroutine 1300 returned for every one-character key
   before testing Tab). It works.
3. `UseItem` while under a guillotine or on a sunk box used a stale target cell. It uses the cell
   in front, as everywhere else.
4. A bump while standing on a sunk box did not turn the player (subroutine 2100 omits 57–60). It
   turns.
5. Outside the level is treated as a wall: a guard against leaving the array. The original
   loaded a level into `ml(1..w, 1..h)` of `DIM ml(100, 100)`; the cells around and beyond it were
   floor, and index −1 or 101 would stop the game with an error. On the shipped levels this is
   reachable only through two edge pockets, both behind boxes that have to be pushed: on level 28
   the top corridor (row 1) ends at the left edge of the map, and on level 25 a room next to the
   start is open at the top edge. In normal play nobody goes there; no LISIY demo leaves the level
   area.
6. The turn keys and Space were not recorded by the 1.1 recorder. Every executed command is
   recorded.
