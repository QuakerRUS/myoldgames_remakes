#pragma once
// Piton rules: one level, one snake, rabbits. Advances one cell per tick().

#include "piton/level.hpp"
#include "piton/types.hpp"

#include <array>
#include <cstdint>
#include <deque>
#include <optional>
#include <vector>

namespace piton {

// Deterministic, platform-independent RNG (SplitMix64).
class Rng {
public:
    explicit Rng(std::uint64_t seed = 0) : state_(seed) {}
    std::uint64_t next();
    // Uniform integer in [0, n), n > 0.
    std::uint32_t below(std::uint32_t n);

private:
    std::uint64_t state_;
};

enum class GameState : std::uint8_t {
    Running,
    GameOver,       // the snake hit the border, a brick or itself
    LevelComplete,  // rabbitsEaten() reached rabbitsToWin(), or no free cell is left for a rabbit
};

enum class DeathCause : std::uint8_t { None, Border, Brick, Self };

enum class EventType : std::uint8_t {
    Turned,         // the heading changed this tick: dir = new, prevDir = old, cell = head before the move
    Moved,          // the head entered `cell` moving `dir`
    Ate,            // a rabbit at `cell` was eaten; value = rabbitsEaten() after it
    RabbitSpawned,  // a new rabbit appeared at `cell`
    Died,           // GameOver: `cell` = cell the head tried to enter (may be outside), `cause`
    LevelComplete,  // value = rabbitsEaten() (less than rabbitsToWin() when the field is full)
};

struct Event {
    EventType type{};
    Cell cell{};
    Dir dir = Dir::Up;
    Dir prevDir = Dir::Up;
    int value = 0;
    DeathCause cause = DeathCause::None;
};

// One body segment for rendering. `in` is the direction the snake travelled to enter this
// cell (from the tail-side neighbour), `out` the direction it travels to leave it (to the
// head-side neighbour). in == out: straight piece; in != out: a turn piece.
// Head: in = out = heading() (direction of the last move). Tail: in = out = direction
// towards the next segment.
struct Segment {
    Cell cell{};
    Dir in = Dir::Up;
    Dir out = Dir::Up;
};

inline constexpr int kMaxQueuedTurns = 2;

class Game {
public:
    // Starts the level: snake on level.startCells(), heading level.startDir(), first rabbit
    // spawned (reported by events()). Whenever no free cell is left for a new rabbit (also
    // right at the start) the level is complete (state LevelComplete, a LevelComplete event):
    // a level too small for rabbitsToWin rabbits ends instead of running on without a rabbit.
    explicit Game(const Level& level, std::uint64_t seed = 1, int rabbitsToWin = kRabbitsPerLevel);

    // --- Input ---------------------------------------------------------------------------
    // Queues a turn to be applied on a coming tick (one queued turn per tick, up to
    // kMaxQueuedTurns). Rejected (returns false) when the queue is full, or when d equals or
    // is opposite to the direction in effect after the already queued turns (180-degree
    // protection: Right then Left is rejected, Up then Left after Right is accepted).
    bool queueTurn(Dir d);
    // Replaces the queue with d (for the AI). Ignored (false) if d is opposite to heading().
    bool steer(Dir d);
    void clearQueuedTurns() { queue_.clear(); }
    int queuedTurns() const { return static_cast<int>(queue_.size()); }
    // Direction the next tick will move: front of the queue, or heading().
    Dir nextHeading() const { return queue_.empty() ? heading_ : queue_.front(); }

    // --- Simulation ----------------------------------------------------------------------
    // Advances one cell. Does nothing when state() != Running. Returns events() of this tick.
    const std::vector<Event>& tick();
    // Events of the last tick() (after construction: the first RabbitSpawned).
    const std::vector<Event>& events() const { return events_; }

    // --- State ---------------------------------------------------------------------------
    GameState state() const { return state_; }
    DeathCause deathCause() const { return death_; }
    const Level& level() const { return level_; }
    Dir heading() const { return heading_; }           // direction of the last move
    const std::deque<Cell>& body() const { return body_; }  // [0] = head, back() = tail
    Cell head() const { return body_.front(); }
    Cell tail() const { return body_.back(); }
    int length() const { return static_cast<int>(body_.size()); }
    std::vector<Segment> segments() const;              // same order as body()
    std::optional<Cell> rabbit() const { return rabbit_; }
    int rabbitsEaten() const { return eaten_; }
    int rabbitsToWin() const { return target_; }
    int ticks() const { return ticks_; }                // successful moves so far

    // Moves the rabbit to c if isFree(c) (tests, editor test-play). Returns success.
    bool placeRabbit(Cell c);

    bool isOnSnake(Cell c) const;
    // Free for a rabbit: inside, not a brick, not on the snake.
    bool isFree(Cell c) const { return level_.isFree(c) && !isOnSnake(c); }

    // --- Smooth rendering helpers --------------------------------------------------------
    // Interpolation for alpha in [0,1) = time since the last tick / tick length:
    // the head is drawn moving from prevHead() to head(); the tail from prevTail() to tail()
    // (prevTail() == tail() when the snake grew on the last tick, i.e. the tail stayed).
    // Before the first tick both equal the current cells.
    Cell prevHead() const { return prevHead_; }
    Cell prevTail() const { return prevTail_; }
    bool grewLastTick() const { return grew_; }

private:
    // Spawns a rabbit on a random free cell; false when there is none.
    bool spawnRabbit();
    void completeLevel(Cell at);
    void setOccupied(Cell c, bool v) {
        if (insideField(c)) occ_[static_cast<size_t>(cellIndex(c))] = v;
    }

    Level level_;
    Rng rng_;
    int target_;
    std::deque<Cell> body_;
    std::array<bool, kCellCount> occ_{};
    Dir heading_;
    std::deque<Dir> queue_;
    std::optional<Cell> rabbit_;
    int eaten_ = 0;
    int ticks_ = 0;
    GameState state_ = GameState::Running;
    DeathCause death_ = DeathCause::None;
    std::vector<Event> events_;
    Cell prevHead_{};
    Cell prevTail_{};
    bool grew_ = false;
};

} // namespace piton
