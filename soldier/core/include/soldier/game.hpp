// The rules of Soldier: an exact port of GAME1.BAS 1.1 on a discrete tick clock.
// See soldier/core/README.md for the timing model and the list of fixed bugs.
#pragma once

#include <optional>
#include <vector>

#include "soldier/command.hpp"
#include "soldier/event.hpp"
#include "soldier/level.hpp"

namespace soldier {

inline constexpr int kInventorySize = 25;
inline constexpr int kPartsPerSecond = 4;  // ticks per game second
inline constexpr int kMoveCooldown = 2;    // ticks between two steps

enum class Status : std::uint8_t { Playing, Won, Dead };

struct RecordedCommand {
    int tick = 0;
    Command command;
    friend bool operator==(const RecordedCommand&, const RecordedCommand&) = default;
};

class Game {
public:
    explicit Game(Level level);

    // ---- input -------------------------------------------------------------------------
    // Processes a command now, inside the current tick. Ignored unless status() == Playing.
    // A Move during the cooldown is buffered (replacing any buffered move) and executed at the
    // start of the first tick where the cooldown has passed; any other command discards the
    // buffered move (as the original's one-key buffer did).
    void apply(Command c);
    // Ends the current tick and starts the next one: guillotines, timer, buffered move.
    // Ignored unless status() == Playing.
    void tick();
    // A key that is not a command (unmapped keys, pause, zoom, ...) was pressed: discards the
    // buffered move, as any key overwrote the original's one-key buffer `i$`. Nothing is
    // executed or recorded.
    void discard_buffered_move() { pending_.reset(); }

    // ---- events ------------------------------------------------------------------------
    // Events produced since the last call, in order.
    std::vector<Event> take_events();

    // ---- state -------------------------------------------------------------------------
    const Level& level() const { return level_; }  // current cells (changes during play)
    int width() const { return level_.width; }
    int height() const { return level_.height; }
    Tile cell(Point p) const { return level_.at(p); }

    Point player() const { return player_; }
    Dir facing() const { return facing_; }
    Ground ground() const { return ground_; }  // what the player stands on

    int unpressed_buttons() const { return onkey_; }  // the original `onkey`
    bool doors_open() const { return onkey_ == 0; }    // plain doors (15) are open

    const std::vector<KeyColor>& inventory() const { return items_; }
    int inventory_cursor() const { return cursor_; }  // 0-based; 0 when empty

    int tick_count() const { return tick_; }                      // ticks since start (= original numsec)
    int part() const { return tick_ % kPartsPerSecond + 1; }      // 1..4 (= original numpartsec)
    bool guillotines_closed() const { return part() == 4; }
    int timer() const { return timer_; }  // timer units left; -1 = no time limit

    // Earliest tick at which a Move is executed immediately.
    int move_ready_tick() const { return move_ready_tick_; }
    bool can_move_now() const { return tick_ >= move_ready_tick_; }
    std::optional<Dir> buffered_move() const { return pending_; }

    Status status() const { return status_; }
    DeathCause death_cause() const { return cause_; }  // valid when Dead
    Tile death_tile() const { return death_tile_; }    // valid when Dead (7, 47, 53, 55, 61, 62)

    // Every command that was executed, with the tick it was executed in. Replaying this list
    // through apply()/tick() reproduces the game exactly.
    const std::vector<RecordedCommand>& history() const { return history_; }

private:
    Level level_;
    Point player_{};
    Dir facing_ = Dir::Down;
    Ground ground_ = Ground::Floor;
    int onkey_ = 0;
    std::vector<KeyColor> items_;
    int cursor_ = 0;
    int tick_ = 0;
    int timer_ = -1;
    int move_ready_tick_ = 0;
    std::optional<Dir> pending_;
    Status status_ = Status::Playing;
    DeathCause cause_ = DeathCause::GaveUp;
    Tile death_tile_ = Tile::Floor;
    std::vector<Event> events_;
    std::vector<RecordedCommand> history_;

    void execute(Command c);
    void move(Dir d);
    bool in_bounds(Point p) const { return level_.in_bounds(p); }
    Tile at_or_wall(Point p) const { return in_bounds(p) ? level_.at(p) : Tile::Wall; }
    void emit(Event e) { events_.push_back(e); }
    void set_facing(Dir d);
    void put_player();
    void leave();                       // original 12100: restore the ground under the player
    void step(Point to, Ground g, Dir d);  // leave + enter `to` standing on `g`
    void bump(Point target, Dir d);     // original 2100
    bool can_push(Point box, Dir d, bool into_water) const;
    void push_box(Point box, Dir d);    // original 12200 (moves the box only)
    void press_button(Point p);
    bool use_key(KeyColor c, Point target);  // original 20000..20500
    void remove_item(int index);
    void die(DeathCause c);
    void turn(bool cw);
};

}  // namespace soldier
