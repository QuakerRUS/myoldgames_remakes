// Per-tile movement and pushing rules (GAME1.BAS 1.1 subroutines 1500..5600).
#include "util.hpp"

using namespace soldier;
using test::go;
using test::has_event;
using test::make_level;
using test::row;

TEST_CASE("floor: step, cooldown set") {
    Game g(make_level({">.."}));
    g.apply(Command::move(Dir::Right));
    CHECK(row(g, 0) == ".>.");
    CHECK(g.player() == Point{1, 0});
    CHECK_FALSE(g.can_move_now());
    CHECK(g.move_ready_tick() == 2);
    const auto ev = g.take_events();
    REQUIRE(ev.size() == 1);
    CHECK(ev[0].type == EventType::Moved);
    CHECK(ev[0].at == Point{0, 0});
    CHECK(ev[0].to == Point{1, 0});
}

TEST_CASE("wall: bump turns the player and costs no cooldown") {
    Game g(make_level({"#v."}));
    g.apply(Command::move(Dir::Left));
    CHECK(row(g, 0) == "#<.");
    CHECK(g.can_move_now());
    const auto ev = g.take_events();
    CHECK(has_event(ev, EventType::Bumped));
    CHECK(has_event(ev, EventType::Turned));
    g.apply(Command::move(Dir::Right));  // same tick
    CHECK(row(g, 0) == "#.>");
}

TEST_CASE("outside the level is a wall") {
    Game g(make_level({"v"}));
    g.apply(Command::move(Dir::Down));
    CHECK(g.player() == Point{0, 0});
    CHECK(g.cell({0, 0}) == Tile::PlayerDown);
    g.apply(Command::move(Dir::Up));
    CHECK(g.cell({0, 0}) == Tile::PlayerUp);
}

TEST_CASE("exit wins") {
    Game g(make_level({">X"}));
    g.apply(Command::move(Dir::Right));
    CHECK(g.status() == Status::Won);
    CHECK(g.player() == Point{1, 0});
    CHECK(row(g, 0) == ".X");
    CHECK(has_event(g.take_events(), EventType::Won));
    g.apply(Command::move(Dir::Left));  // ignored after the end
    CHECK(g.player() == Point{1, 0});
}

TEST_CASE("box: pushed onto floor, blocked by wall, box, key, exit") {
    Game g(make_level({">B.#"}));
    g.apply(Command::move(Dir::Right));
    CHECK(row(g, 0) == ".>B#");
    go(g, Dir::Right);
    CHECK(row(g, 0) == ".>B#");  // blocked by the wall
    CHECK(g.can_move_now());
    for (const char* blocker : {"BB", "Br", "BX", "B%", "B?", "BT"}) {
        CAPTURE(blocker);
        Game h(make_level({std::string(">") + blocker}));
        h.apply(Command::move(Dir::Right));
        CHECK(h.player() == Point{0, 0});
    }
}

TEST_CASE("box onto a button presses it; pushing it off releases nothing (the player stands there)") {
    Game g(make_level({">B_.", "D..."}));
    CHECK_FALSE(g.doors_open());
    g.apply(Command::move(Dir::Right));
    CHECK(row(g, 0) == ".>b.");
    CHECK(g.doors_open());
    const auto ev = g.take_events();
    CHECK(has_event(ev, EventType::ButtonPressed));
    CHECK(has_event(ev, EventType::DoorsOpened));
    go(g, Dir::Right);  // push the box off the button: player now on the button
    CHECK(row(g, 0) == "..4B");
    CHECK(g.doors_open());
    go(g, Dir::Left);  // leave the button
    CHECK(row(g, 0) == ".<_B");
    CHECK_FALSE(g.doors_open());
    CHECK(has_event(g.take_events(), EventType::DoorsClosed));
}

TEST_CASE("door: blocked while a button is free, open when all are pressed") {
    Game g(make_level({"_<D."}));
    g.apply(Command::move(Dir::Right));
    CHECK(g.player() == Point{1, 0});  // bump
    CHECK(g.facing() == Dir::Right);
    g.apply(Command::move(Dir::Left));
    CHECK(row(g, 0) == "3.D.");
    go(g, Dir::Right);  // leaving the button closes the door again
    go(g, Dir::Right);
    CHECK(g.player() == Point{1, 0});
}

TEST_CASE("door: walk through when open; box pushed into an open door") {
    Game g(make_level({">DD."}));  // no buttons at all: doors open
    CHECK(g.doors_open());
    g.apply(Command::move(Dir::Right));
    CHECK(row(g, 0) == ".8D.");
    go(g, Dir::Right);
    CHECK(row(g, 0) == ".D8.");
    Game h(make_level({">BD"}));
    h.apply(Command::move(Dir::Right));
    CHECK(row(h, 0) == ".>d");
    Game k(make_level({">d.."}));  // a box in a door can be pushed out
    k.apply(Command::move(Dir::Right));
    CHECK(row(k, 0) == ".8B.");
    go(k, Dir::Right);
    CHECK(row(k, 0) == ".D>B");
}

TEST_CASE("door: box cannot enter a closed door, but a box already in it can still be pushed") {
    Game g(make_level({">BD_"}));
    g.apply(Command::move(Dir::Right));
    CHECK(g.player() == Point{0, 0});
    Game h(make_level({">d._"}));  // original quirk: 2700 does not check the door's own state
    h.apply(Command::move(Dir::Right));
    CHECK(row(h, 0) == ".8B_");
}

TEST_CASE("box on button: cannot be pushed into water or onto a sunk box") {
    for (const char* beyond : {"~", "="}) {
        CAPTURE(beyond);
        Game g(make_level({std::string(">b") + beyond}));
        g.apply(Command::move(Dir::Right));
        CHECK(g.player() == Point{0, 0});
    }
}

TEST_CASE("water: a pushed box sinks; the sunk box is walkable; a box on it can be pushed on") {
    Game g(make_level({">B~.."}));
    g.apply(Command::move(Dir::Right));
    CHECK(row(g, 0) == ".>=..");
    const auto ev = g.take_events();
    CHECK(has_event(ev, EventType::BoxPushed));
    CHECK(has_event(ev, EventType::BoxSank));
    go(g, Dir::Right);
    CHECK(row(g, 0) == "..]..");
    CHECK(g.ground() == Ground::SunkBox);
    go(g, Dir::Right);
    CHECK(row(g, 0) == "..=>.");
}

TEST_CASE("sunk box: a box pushed onto it becomes a box on a sunk box, and can be pushed off") {
    Game g(make_level({"v..", "B..", "=..", "..."}));
    g.apply(Command::move(Dir::Down));
    CHECK(row(g, 1) == "v..");
    CHECK(row(g, 2) == "&..");
    go(g, Dir::Down);  // push the box off the sunk box: player on the sunk box
    CHECK(row(g, 2) == "(..");
    CHECK(row(g, 3) == "B..");
}

TEST_CASE("push right onto a sunk box works (subroutine 2300 bug fixed)") {
    // The original checked ml(absx - 2, absy) instead of ml(absx + 2, absy) when pushing right.
    Game g(make_level({".>B=."}));
    g.apply(Command::move(Dir::Right));
    CHECK(row(g, 0) == "..>&.");
    Game h(make_level({".>&=."}));  // and subroutine 5600
    h.apply(Command::move(Dir::Right));
    CHECK(row(h, 0) == "..]&.");
    Game k(make_level({"=.>B#"}));  // the bug let this box vanish into the wall
    k.apply(Command::move(Dir::Right));
    CHECK(row(k, 0) == "=.>B#");
}

TEST_CASE("water kills") {
    Game g(make_level({">~"}));
    g.apply(Command::move(Dir::Right));
    CHECK(g.status() == Status::Dead);
    CHECK(g.death_cause() == DeathCause::Drowned);
    CHECK(g.death_tile() == Tile::DeathDrowned);
    CHECK(row(g, 0) == ".*");
    CHECK(g.player() == Point{1, 0});
}

TEST_CASE("dark wall becomes a wall when bumped; fake wall is walkable and disappears") {
    Game g(make_level({"%v?."}));
    g.apply(Command::move(Dir::Left));
    CHECK(row(g, 0) == "#<?.");
    CHECK(has_event(g.take_events(), EventType::WallRevealed));
    g.apply(Command::move(Dir::Right));
    CHECK(row(g, 0) == "#.>.");
    CHECK(has_event(g.take_events(), EventType::FakeWallEntered));
    go(g, Dir::Right);
    CHECK(row(g, 0) == "#..>");
}

TEST_CASE("decorations (player-like and death tiles) are inert") {
    Game g(make_level({"v<", "!."}));  // start = last player tile = (1,0)
    g.apply(Command::move(Dir::Left));
    CHECK(g.player() == Point{1, 0});
    CHECK(g.facing() == Dir::Left);
    g.apply(Command::move(Dir::Down));
    go(g, Dir::Left);
    CHECK(g.player() == Point{1, 1});
    CHECK(g.facing() == Dir::Down);  // no turn into an inert tile
    CHECK(g.cell({0, 1}) == Tile::DeathGaveUp);
}

TEST_CASE("bumping on a sunk box turns the player (the original did not)") {
    Game g(make_level({"#=<"}));
    g.apply(Command::move(Dir::Left));
    CHECK(g.cell({1, 0}) == Tile::PlayerOnSunkBoxLeft);
    go(g, Dir::Up);  // outside the level = wall
    CHECK(g.cell({1, 0}) == Tile::PlayerOnSunkBoxUp);
}

TEST_CASE("give up: death tile depends on the ground") {
    Game a(make_level({"v"}));
    a.apply(Command::give_up());
    CHECK(a.status() == Status::Dead);
    CHECK(a.death_cause() == DeathCause::GaveUp);
    CHECK(a.death_tile() == Tile::DeathGaveUp);
    CHECK(a.cell({0, 0}) == Tile::DeathGaveUp);
    Game c(make_level({">="}));
    c.apply(Command::move(Dir::Right));
    c.apply(Command::give_up());
    CHECK(c.death_tile() == Tile::DeathGaveUpOnSunkBox);
    Game d(make_level({"}"}));
    d.apply(Command::give_up());
    CHECK(d.death_tile() == Tile::DeathGuillotine);
    CHECK(d.death_cause() == DeathCause::GaveUp);
}
