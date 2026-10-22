// The tick clock: move cooldown and buffering, guillotines, timer.
#include "util.hpp"

using namespace soldier;
using test::has_event;
using test::make_level;
using test::row;

TEST_CASE("parts of a second") {
    Game g(make_level({"v"}));
    CHECK(g.tick_count() == 0);
    CHECK(g.part() == 1);
    for (int t = 1; t <= 9; ++t) {
        g.tick();
        CHECK(g.tick_count() == t);
        CHECK(g.part() == t % 4 + 1);
        CHECK(g.guillotines_closed() == (t % 4 == 3));
    }
}

TEST_CASE("a move during the 2-tick cooldown is buffered and executed when it ends") {
    Game g(make_level({">...."}));
    g.apply(Command::move(Dir::Right));  // tick 0
    g.apply(Command::move(Dir::Right));  // buffered
    CHECK(g.player() == Point{1, 0});
    CHECK(g.buffered_move() == Dir::Right);
    g.tick();  // tick 1: still cooling down
    CHECK(g.player() == Point{1, 0});
    g.tick();  // tick 2: executed at the start of the tick
    CHECK(g.player() == Point{2, 0});
    CHECK_FALSE(g.buffered_move().has_value());
    REQUIRE(g.history().size() == 2);
    CHECK(g.history()[0] == RecordedCommand{0, Command::move(Dir::Right)});
    CHECK(g.history()[1] == RecordedCommand{2, Command::move(Dir::Right)});
}

TEST_CASE("the last buffered move wins; any other command discards it") {
    Game g(make_level({".....", "..>..", "....."}));
    g.apply(Command::move(Dir::Right));
    g.apply(Command::move(Dir::Down));
    g.apply(Command::move(Dir::Up));
    g.tick();
    g.tick();
    CHECK(g.player() == Point{3, 0});
    g.apply(Command::move(Dir::Down));  // buffered
    g.apply(Command::turn_cw());        // discards it
    g.tick();
    g.tick();
    CHECK(g.player() == Point{3, 0});
    CHECK(g.facing() == Dir::Right);  // up turned clockwise
}

TEST_CASE("any other key discards the buffered move without recording anything") {
    Game g(make_level({">...."}));
    g.apply(Command::move(Dir::Right));
    g.apply(Command::move(Dir::Right));  // buffered
    g.discard_buffered_move();           // e.g. an unmapped key
    CHECK_FALSE(g.buffered_move().has_value());
    g.tick();
    g.tick();
    CHECK(g.player() == Point{1, 0});
    REQUIRE(g.history().size() == 1);
    CHECK(g.history()[0] == RecordedCommand{0, Command::move(Dir::Right)});
}

TEST_CASE("turning, items and giving up ignore the cooldown") {
    Game g(make_level({">r.."}));
    g.apply(Command::move(Dir::Right));
    g.apply(Command::turn_ccw());
    CHECK(g.facing() == Dir::Up);
    g.apply(Command::turn_cw());
    g.apply(Command::use_item());
    CHECK(row(g, 0) == ".>r.");
    g.apply(Command::give_up());
    CHECK(g.status() == Status::Dead);
}

TEST_CASE("guillotine: cannot be entered while closed (4th part)") {
    Game g(make_level({">T."}));
    g.tick();
    g.tick();
    g.tick();  // part 4
    CHECK(g.guillotines_closed());
    g.apply(Command::move(Dir::Right));
    CHECK(g.player() == Point{0, 0});
    g.tick();  // part 1 of the next second
    CHECK(has_event(g.take_events(), EventType::GuillotinesOpened));
    g.apply(Command::move(Dir::Right));
    CHECK(row(g, 0) == ".}.");
    CHECK(g.ground() == Ground::Guillotine);
    g.tick();
    g.tick();
    g.apply(Command::move(Dir::Right));  // leave in part 3: safe
    CHECK(row(g, 0) == ".T>");
    g.tick();
    CHECK(g.status() == Status::Playing);
}

TEST_CASE("guillotine: closing on the player kills") {
    Game g(make_level({">T."}));
    g.apply(Command::move(Dir::Right));  // part 1
    g.tick();
    g.tick();
    CHECK(g.status() == Status::Playing);
    g.tick();  // part 4 begins
    CHECK(g.status() == Status::Dead);
    CHECK(g.death_cause() == DeathCause::Guillotine);
    CHECK(g.death_tile() == Tile::DeathGuillotine);
    CHECK(g.cell({1, 0}) == Tile::DeathGuillotine);
    const auto ev = g.take_events();
    CHECK(has_event(ev, EventType::GuillotinesClosed));
    CHECK(has_event(ev, EventType::Died));
}

TEST_CASE("timer: decreases every 4 ticks; death when a second ends at 0") {
    Game g(make_level({"v"}, 2));
    CHECK(g.timer() == 2);
    for (int i = 0; i < 4; ++i) g.tick();
    CHECK(g.timer() == 1);
    CHECK(has_event(g.take_events(), EventType::TimerChanged));
    for (int i = 0; i < 4; ++i) g.tick();
    CHECK(g.timer() == 0);
    CHECK(g.status() == Status::Playing);
    for (int i = 0; i < 3; ++i) g.tick();
    CHECK(g.status() == Status::Playing);
    g.tick();  // tick 12 = 4 * (2 + 1)
    CHECK(g.status() == Status::Dead);
    CHECK(g.death_cause() == DeathCause::TimeOut);
    CHECK(g.death_tile() == Tile::DeathTimeOut);
    CHECK(g.tick_count() == 12);
}

TEST_CASE("timer: time-out tiles on a sunk box and under a guillotine") {
    Game a(make_level({">="}, 1));
    a.apply(Command::move(Dir::Right));
    for (int i = 0; i < 8; ++i) a.tick();
    CHECK(a.death_tile() == Tile::DeathTimeOutOnSunkBox);
    CHECK(a.death_cause() == DeathCause::TimeOut);
}

TEST_CASE("no time limit") {
    Game g(make_level({"v"}));
    CHECK(g.timer() == -1);
    for (int i = 0; i < 1000; ++i) g.tick();
    CHECK(g.status() == Status::Playing);
    CHECK(g.timer() == -1);
}
