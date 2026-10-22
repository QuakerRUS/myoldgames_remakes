# piton_core

SDL-free logic of Piton: levels, game rules, and the demo AI. Static library `piton_core`
(C++20, namespace `piton`, headers under `piton/core/include/piton/`). No dependencies.

```cmake
target_link_libraries(my_target PRIVATE piton_core)
```

Headers: `#include <piton/types.hpp>`, `<piton/level.hpp>`, `<piton/game.hpp>`,
`<piton/demo_pilot.hpp>` (each includes what it needs).

Level files: `piton/data/levels/01.json` … `10.json`, format in
[docs/formats/piton-level.md](../../docs/formats/piton-level.md).

## Coordinates and directions (`types.hpp`)

```cpp
constexpr int kFieldWidth = 38, kFieldHeight = 23;   // x = 0..37 left→right, y = 0..22 top→bottom
constexpr int kCellCount = 38 * 23;
constexpr int kStartLength = 3;                       // segments at level start
constexpr int kRabbitsPerLevel = 50;
constexpr int kLevelCount = 10;

enum class Dir : uint8_t { Up = 1, Right = 2, Down = 3, Left = 4 };  // original numbering
constexpr Dir kAllDirs[4];                            // Up, Right, Down, Left

struct Cell { int x, y; };                            // operator== defined

Dir opposite(Dir);
int dx(Dir); int dy(Dir);                             // Right: dx=+1, Down: dy=+1
Cell step(Cell, Dir);                                 // neighbour (may be outside)
bool insideField(Cell);
int cellIndex(Cell);  Cell cellAt(int index);         // row-major: y*38 + x
std::optional<Dir> dirBetween(Cell a, Cell b);        // direction a→b for orthogonal neighbours
std::string_view dirName(Dir);                        // "up" "right" "down" "left"
std::optional<Dir> dirFromName(std::string_view);
```

The field has an implicit border around the 38×23 grid. In the original's 320×200 screen
each cell is 8×8 pixels; the renderer is free to choose its own scale.

## Level (`level.hpp`)

```cpp
enum class Tile : uint8_t { Empty = 0, Brick = 1 };

class Level {
public:
    Level();                                  // all empty; start head (20,18) facing Right
    std::string name;

    Tile tile(Cell) const;                    // outside the field → Brick
    bool isBrick(Cell) const;
    bool isFree(Cell) const;                  // inside and Empty
    void setTile(Cell, Tile);                 // ignored outside
    void fill(Tile);

    Cell startHead() const;
    Dir  startDir() const;
    bool setStart(Cell head, Dir dir);        // false (unchanged) if a start cell is outside;
                                              // clears bricks under the start
    std::array<Cell, 3> startCells() const;   // head first; the body lies straight behind it
    static std::array<Cell, 3> startCellsFor(Cell head, Dir dir);
    static bool startFits(Cell head, Dir dir);   // all 3 cells inside the field
    static Cell clampStart(Cell head, Dir dir);  // nearest head that fits (the editor uses it)

    int freeCellCount() const;
    std::vector<std::string> rows() const;    // 23 strings of 38 chars, '#' / '.'

    static bool fromJson(std::string_view json, Level& out, std::string* error = nullptr);
    std::string toJson() const;               // canonical form (the shipped files are in it)
    static bool load(const std::filesystem::path&, Level& out, std::string* error = nullptr);
    bool save(const std::filesystem::path&, std::string* error = nullptr) const;
};
bool operator==(const Level&, const Level&);  // compares tiles, start and name

struct LevelCheck {
    bool ok() const;                          // problems.empty()
    bool playable() const;                    // errors.empty()
    std::vector<std::string> problems;        // errors + warnings, English, ready to show
    std::vector<std::string> errors;          // hard: cannot be played / completed
    std::vector<std::string> warnings;        // design rules
    int freeCells, regions, longestDeadEnd;
};
LevelCheck checkLevel(const Level&);
std::string levelFileName(int number);        // 1 → "01.json"

constexpr int kMinFreeAhead = 6, kMaxDeadEndLength = 3, kMinFreeCells = 159;
```

`checkLevel` hard errors (`errors`): a start cell outside the field or not free; no free cell
for a rabbit; the empty cells not forming exactly one connected region (a rabbit could appear
where the snake cannot reach it). Warnings: fewer than 6 free cells ahead of the head; fewer
than `kMinFreeCells` empty cells; a 1-cell-wide dead end longer than 3 cells. `piton.exe
--test-level` refuses a level with errors and plays one with warnings. `fromJson`/`load`
validate the syntax (sizes, characters) and that all three start cells lie inside the field;
bricks under the start cells are removed on load. Otherwise they do not apply these rules, so
the editor can open an invalid level and show `problems`.

## Game (`game.hpp`)

```cpp
class Rng { explicit Rng(uint64_t seed = 0); uint64_t next(); uint32_t below(uint32_t n); };  // SplitMix64

enum class GameState : uint8_t { Running, GameOver, LevelComplete };
enum class DeathCause : uint8_t { None, Border, Brick, Self };

enum class EventType : uint8_t { Turned, Moved, Ate, RabbitSpawned, Died, LevelComplete };
struct Event {
    EventType type; Cell cell; Dir dir; Dir prevDir; int value; DeathCause cause;
};

struct Segment { Cell cell; Dir in; Dir out; };
constexpr int kMaxQueuedTurns = 2;

class Game {
public:
    explicit Game(const Level&, uint64_t seed = 1, int rabbitsToWin = kRabbitsPerLevel);

    // input
    bool queueTurn(Dir);          // player input (arrow keys)
    bool steer(Dir);              // AI input: replaces the queue
    void clearQueuedTurns();
    int  queuedTurns() const;
    Dir  nextHeading() const;     // what the next tick will use

    // simulation
    const std::vector<Event>& tick();     // one cell; no-op unless Running
    const std::vector<Event>& events() const;

    // state
    GameState state() const;  DeathCause deathCause() const;
    const Level& level() const;
    Dir heading() const;                       // direction of the last move
    const std::deque<Cell>& body() const;      // [0] head … back() tail
    Cell head() const;  Cell tail() const;  int length() const;
    std::vector<Segment> segments() const;     // same order as body()
    std::optional<Cell> rabbit() const;        // nullopt after LevelComplete
    int rabbitsEaten() const;  int rabbitsToWin() const;
    int ticks() const;                         // moves made so far
    bool isOnSnake(Cell) const;
    bool isFree(Cell) const;                   // empty level cell not on the snake
    bool placeRabbit(Cell);                    // tests / editor test-play: move the rabbit

    // smooth rendering
    Cell prevHead() const;  Cell prevTail() const;  bool grewLastTick() const;
};
```

### Rules

- A `Game` is one attempt at one level. Start: 3 segments from `Level::startCells()`,
  heading `startDir()`, the first rabbit already placed (`events()` holds its
  `RabbitSpawned`). For the next level or a retry, construct a new `Game`.
- `tick()` moves the snake exactly one cell. The library has no clock: the app calls it at
  a fixed rate; the design target is about 8 ticks per second (125 ms).
- Moving into a cell outside the field (`Border`), a brick (`Brick`) or a body segment
  (`Self`) ends the game (`GameOver`); the snake does not move on that tick. Moving into
  the cell the tail is just leaving is allowed (unless the snake grows on that tick).
- Entering the rabbit's cell eats it: the snake grows by one (the tail stays in place that
  tick), `rabbitsEaten()` increases, a new rabbit appears on a uniformly random cell that
  is empty and not on the snake. When `rabbitsEaten() == rabbitsToWin()` (50) the state is
  `LevelComplete` and no new rabbit appears. When no free cell is left for a new rabbit
  (a level too small for 50 rabbits, or no free cell at all at the start) the level is
  complete too, with fewer rabbits eaten: it never runs on without a rabbit.
- The rabbit sequence depends only on the seed and the moves (`Rng` is SplitMix64, the
  same on every platform), so a recorded input sequence replays exactly.

### Input

`queueTurn(d)` stores up to 2 turns; each tick consumes one. A turn is rejected (returns
false) when the queue is full or when `d` equals or reverses the direction in effect after
the turns already queued: heading Right, `queueTurn(Left)` is rejected, but
`queueTurn(Up)` then `queueTurn(Left)` is accepted and performs a quick U-turn over two
ticks. Repeated presses of the current direction are ignored. Call it on key-down.

`steer(d)` (for the AI) clears the queue and sets `d` for the next tick; it is rejected only
for the reverse of `heading()`.

### Events

`tick()` clears and refills `events()`. Order within one tick:

| Type | When | Fields |
|---|---|---|
| `Turned` | the heading changed | `cell` = head before the move, `dir` new, `prevDir` old |
| `Died` | collision (then nothing else) | `cell` = cell the head tried to enter (may be outside), `dir`, `cause` |
| `Moved` | the head entered a cell | `cell` = new head, `dir` |
| `Ate` | a rabbit was eaten | `cell`, `value` = rabbits eaten so far |
| `RabbitSpawned` | new rabbit | `cell` |
| `LevelComplete` | target reached | `cell` = head, `value` = rabbits eaten |

Suggested sounds: Turned → turn, Ate → eat, Died → death, LevelComplete → fanfare.

### Rendering the snake

`segments()` gives every cell with two directions: `in` — the direction the snake travelled
to enter this cell (from the tail-side neighbour), `out` — the direction it leaves it (to
the head-side neighbour).

- `segments()[0]` is the head: `in == out == heading()`. Use `out` to pick the head sprite
  (HEAD*U/R/D/L). After `GameOver` by a turn into a wall, `heading()` is the new direction,
  so the head faces what it hit.
- Middle segments: `in == out` → straight piece (vertical for Up/Down, horizontal for
  Left/Right); `in != out` → turn piece; the pair (in, out) names the corner, e.g.
  in = Right, out = Down is the corner that connects the left neighbour with the lower one.
- `segments().back()` is the tail: `in == out` = direction towards the next segment; pick the
  tail sprite pointing away from it.

Smooth movement between ticks: let `alpha` = time since the last tick / tick length, in
[0, 1). Draw the head sliding from `prevHead()` to `head()` (i.e. at
`lerp(prevHead, head, alpha)`), and the tail sliding from `prevTail()` to `tail()`; when
`grewLastTick()` the tail did not move (`prevTail() == tail()`). Middle segments stay on
their cells (draw the head-side half of `segments()[1]` growing with alpha if you want a
continuous body). After `GameOver` nothing moves: `prevHead() == head()`.
Cells convert to pixels as `x * cellSize`, `y * cellSize` inside the border.

## DemoPilot (`demo_pilot.hpp`)

```cpp
class DemoPilot {
public:
    explicit DemoPilot(uint64_t seed = 0x5EED);
    Dir choose(const Game&);       // direction for the next tick, never the reverse
    void drive(Game& g);           // g.steer(choose(g))
};
```

Use one `DemoPilot` per `Game`; before every `tick()` call `pilot.drive(game)`:

```cpp
piton::Game game(level, seed);
piton::DemoPilot pilot;
// every 125 ms:
pilot.drive(game);
for (const piton::Event& e : game.tick()) { /* sounds, effects */ }
if (game.state() == piton::GameState::LevelComplete) { /* next level */ }
```

Strategy: shortest path to the rabbit (time-aware BFS: body cells count as free once the
tail has passed; straight runs preferred), taken only if the snake can still reach its own
tail after eating; otherwise it follows its tail (keeping the tail reachable) until a safe
path appears, with an occasional random move after a long time without food to break
loops; when even that fails, the move with the largest reachable area (flood fill). The
tests show it completing every shipped level (50 rabbits) for many seeds (2000 runs, 200 seeds per level, no failure) in
about 1000–3200 ticks (2–7 minutes at 8 ticks/s).

## Tests

`piton_core_tests` (doctest, run from the repository root or anywhere — the level directory
is compiled in): rules, the level checks for all 10 levels, JSON round trip, and the
DemoPilot simulation on every level. It prints per-level statistics.

```
cmake --preset vs2022 -B build/piton-core -DVS_LOGIC_ONLY=ON
cmake --build build/piton-core --config Debug --target piton_core_tests
ctest --test-dir build/piton-core -C Debug -R piton_core
```
