// Coloured keys and doors, inventory (subroutines 1950, 1960, 2800..5100, 12300, 20000..20500).
#include "util.hpp"

using namespace soldier;
using test::go;
using test::has_event;
using test::make_level;
using test::row;

TEST_CASE("picking a key adds it to the inventory") {
    Game g(make_level({">rg."}));
    g.apply(Command::move(Dir::Right));
    CHECK(row(g, 0) == ".>g.");
    go(g, Dir::Right);
    REQUIRE(g.inventory().size() == 2);
    CHECK(g.inventory()[0] == KeyColor::Red);
    CHECK(g.inventory()[1] == KeyColor::Green);
    CHECK(g.inventory_cursor() == 0);
    CHECK(has_event(g.take_events(), EventType::KeyPicked));
}

TEST_CASE("a coloured door needs a key of its colour and consumes it") {
    Game g(make_level({">gR."}));
    go(g, Dir::Right);  // green key
    go(g, Dir::Right);  // red door: no red key
    CHECK(g.player() == Point{1, 0});
    CHECK(g.cell({2, 0}) == Tile::RedDoor);
    Game h(make_level({">grR."}));
    go(h, Dir::Right);
    go(h, Dir::Right);
    go(h, Dir::Right);  // opens with the red key (the first red one), walks in
    CHECK(row(h, 0) == "...>.");
    REQUIRE(h.inventory().size() == 1);
    CHECK(h.inventory()[0] == KeyColor::Green);
    CHECK(h.inventory_cursor() == 0);
    CHECK(has_event(h.take_events(), EventType::ColorDoorUnlocked));
    go(h, Dir::Left);
    CHECK(row(h, 0) == "..<..");  // the door is gone
}

TEST_CASE("key on a button: picking it keeps the button pressed; key in a door is picked even when closed") {
    Game g(make_level({">a.D_"}));
    CHECK(g.unpressed_buttons() == 1);
    g.apply(Command::move(Dir::Right));
    CHECK(row(g, 0) == ".4.D_");
    CHECK(g.unpressed_buttons() == 1);
    CHECK(g.inventory().size() == 1);
    go(g, Dir::Right);  // leave the button: it is free again
    CHECK(g.unpressed_buttons() == 2);
    Game h(make_level({">A_"}));  // door closed (a free button), the key in it is still taken
    h.apply(Command::move(Dir::Right));
    CHECK(row(h, 0) == ".8_");
    CHECK(h.inventory().size() == 1);
}

TEST_CASE("use: drop the selected key on the floor in front") {
    Game g(make_level({">r.."}));
    g.apply(Command::move(Dir::Right));
    g.apply(Command::use_item());
    CHECK(row(g, 0) == ".>r.");
    CHECK(g.inventory().empty());
    const auto ev = g.take_events();
    CHECK(has_event(ev, EventType::KeyDropped));
    g.apply(Command::use_item());  // nothing to use
    CHECK(row(g, 0) == ".>r.");
}

TEST_CASE("use: a key dropped on a button presses it; into an open door; opens a coloured door") {
    Game g(make_level({">r_D", "...."}));
    g.apply(Command::move(Dir::Right));
    g.apply(Command::use_item());
    CHECK(row(g, 0) == ".>aD");
    CHECK(g.doors_open());
    CHECK(has_event(g.take_events(), EventType::DoorsOpened));

    Game h(make_level({">rD"}));
    h.apply(Command::move(Dir::Right));
    h.apply(Command::use_item());
    CHECK(row(h, 0) == ".>A");

    Game k(make_level({">rD_"}));  // closed door: nothing happens
    k.apply(Command::move(Dir::Right));
    k.apply(Command::use_item());
    CHECK(row(k, 0) == ".>D_");
    CHECK(k.inventory().size() == 1);

    Game m(make_level({">rR."}));
    m.apply(Command::move(Dir::Right));
    m.apply(Command::use_item());
    CHECK(row(m, 0) == ".>..");
    CHECK(m.inventory().empty());

    Game n(make_level({">rG#"}));  // wrong colour / wall: nothing
    n.apply(Command::move(Dir::Right));
    n.apply(Command::use_item());
    CHECK(row(n, 0) == ".>G#");
}

TEST_CASE("use acts in the facing direction, also from a sunk box or a guillotine") {
    Game g(make_level({"v", "r", "=", "."}));
    g.apply(Command::move(Dir::Down));
    go(g, Dir::Down);
    CHECK(g.ground() == Ground::SunkBox);
    g.apply(Command::use_item());
    CHECK(g.cell({0, 3}) == Tile::RedKey);
}

TEST_CASE("next item cycles; use takes the selected item and resets the cursor") {
    Game g(make_level({">rgu.", "....."}));
    go(g, Dir::Right);
    go(g, Dir::Right);
    go(g, Dir::Right);
    REQUIRE(g.inventory().size() == 3);
    g.apply(Command::next_item());
    CHECK(g.inventory_cursor() == 1);
    g.apply(Command::next_item());
    g.apply(Command::next_item());
    CHECK(g.inventory_cursor() == 0);  // wraps
    g.apply(Command::next_item());
    g.apply(Command::use_item());  // drops the green key
    CHECK(g.cell({4, 0}) == Tile::GreenKey);
    REQUIRE(g.inventory().size() == 2);
    CHECK(g.inventory()[0] == KeyColor::Red);
    CHECK(g.inventory()[1] == KeyColor::Blue);
    CHECK(g.inventory_cursor() == 0);
}

TEST_CASE("inventory holds at most 25 items; a 26th key blocks like a wall") {
    std::string line = ">" + std::string(26, 'r') + ".";
    Game g(make_level({line}));
    for (int i = 0; i < 26; ++i) go(g, Dir::Right);
    CHECK(g.inventory().size() == kInventorySize);
    CHECK(g.player() == Point{25, 0});
    CHECK(g.cell({26, 0}) == Tile::RedKey);
    Game h(make_level({">" + std::string(25, 'r') + "a"}));  // key on a button too
    for (int i = 0; i < 26; ++i) go(h, Dir::Right);
    CHECK(h.player() == Point{25, 0});
    Game k(make_level({">" + std::string(25, 'r') + "R"}));  // a door still opens with a full inventory
    for (int i = 0; i < 26; ++i) go(k, Dir::Right);
    CHECK(k.player() == Point{26, 0});
    CHECK(k.inventory().size() == 24);
}
