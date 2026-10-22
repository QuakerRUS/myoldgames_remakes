#include <doctest/doctest.h>
#include <piton/game.hpp>

#include <algorithm>

using namespace piton;

namespace {

Level openLevel(Cell head, Dir dir) {
    Level lv;
    lv.setStart(head, dir);
    return lv;
}

bool hasEvent(const std::vector<Event>& ev, EventType t) {
    return std::any_of(ev.begin(), ev.end(), [t](const Event& e) { return e.type == t; });
}

const Event* findEvent(const std::vector<Event>& ev, EventType t) {
    for (const Event& e : ev)
        if (e.type == t) return &e;
    return nullptr;
}

// Moves the rabbit somewhere harmless, far from the snake's path.
void parkRabbit(Game& g) {
    for (int i = kCellCount - 1; i >= 0; --i)
        if (g.placeRabbit(cellAt(i))) return;
}

} // namespace

TEST_CASE("start position: three segments behind the head") {
    Game g(openLevel({10, 5}, Dir::Right), 7);
    REQUIRE(g.length() == 3);
    CHECK(g.head() == Cell{10, 5});
    CHECK(g.body()[1] == Cell{9, 5});
    CHECK(g.tail() == Cell{8, 5});
    CHECK(g.heading() == Dir::Right);
    CHECK(g.state() == GameState::Running);
    REQUIRE(g.rabbit());
    CHECK(hasEvent(g.events(), EventType::RabbitSpawned));
    CHECK(g.prevHead() == g.head());
}

TEST_CASE("one cell per tick, Moved event") {
    Game g(openLevel({10, 5}, Dir::Right), 7);
    parkRabbit(g);
    const auto& ev = g.tick();
    REQUIRE(hasEvent(ev, EventType::Moved));
    CHECK(g.head() == Cell{11, 5});
    CHECK(g.tail() == Cell{9, 5});
    CHECK(g.prevHead() == Cell{10, 5});
    CHECK(g.prevTail() == Cell{8, 5});
    CHECK(g.length() == 3);
    CHECK(g.ticks() == 1);
}

TEST_CASE("eating grows the snake by one") {
    Game g(openLevel({10, 5}, Dir::Right), 7);
    REQUIRE(g.placeRabbit({11, 5}));
    const auto& ev = g.tick();
    const Event* ate = findEvent(ev, EventType::Ate);
    REQUIRE(ate);
    CHECK(ate->cell == Cell{11, 5});
    CHECK(ate->value == 1);
    CHECK(g.rabbitsEaten() == 1);
    CHECK(g.length() == 4);
    CHECK(g.tail() == Cell{8, 5});
    CHECK(g.grewLastTick());
    CHECK(g.prevTail() == g.tail());
    CHECK(hasEvent(ev, EventType::RabbitSpawned));
    REQUIRE(g.rabbit());
    CHECK(g.isFree(*g.rabbit()));
}

TEST_CASE("180-degree turns are rejected, two turns can be queued") {
    Game g(openLevel({10, 10}, Dir::Right), 7);
    parkRabbit(g);
    CHECK_FALSE(g.queueTurn(Dir::Left));   // reverse
    CHECK_FALSE(g.queueTurn(Dir::Right));  // same direction
    CHECK(g.queueTurn(Dir::Up));
    CHECK_FALSE(g.queueTurn(Dir::Down));   // reverse of the queued Up
    CHECK(g.queueTurn(Dir::Left));         // Up then Left: a quick U-turn
    CHECK_FALSE(g.queueTurn(Dir::Down));   // queue full
    CHECK(g.queuedTurns() == 2);
    CHECK(g.nextHeading() == Dir::Up);

    auto ev = g.tick();
    const Event* t = findEvent(ev, EventType::Turned);
    REQUIRE(t);
    CHECK(t->prevDir == Dir::Right);
    CHECK(t->dir == Dir::Up);
    CHECK(t->cell == Cell{10, 10});
    CHECK(g.head() == Cell{10, 9});
    g.tick();
    CHECK(g.heading() == Dir::Left);
    CHECK(g.head() == Cell{9, 9});
    CHECK(g.state() == GameState::Running);
    g.tick();
    CHECK_FALSE(hasEvent(g.events(), EventType::Turned));
    CHECK(g.head() == Cell{8, 9});
}

TEST_CASE("steer replaces the queue and refuses reversing") {
    Game g(openLevel({10, 10}, Dir::Right), 7);
    CHECK_FALSE(g.steer(Dir::Left));
    CHECK(g.nextHeading() == Dir::Right);
    CHECK(g.steer(Dir::Down));
    CHECK(g.nextHeading() == Dir::Down);
}

TEST_CASE("hitting the border is game over") {
    Game g(openLevel({36, 3}, Dir::Right), 7);
    parkRabbit(g);
    if (g.rabbit() && g.rabbit()->x == 37 && g.rabbit()->y == 3) g.placeRabbit({0, 22});
    g.tick();
    CHECK(g.head() == Cell{37, 3});
    const auto& ev = g.tick();
    CHECK(g.state() == GameState::GameOver);
    CHECK(g.deathCause() == DeathCause::Border);
    const Event* died = findEvent(ev, EventType::Died);
    REQUIRE(died);
    CHECK(died->cell == Cell{38, 3});
    CHECK(died->cause == DeathCause::Border);
    // Nothing happens after the end.
    CHECK(g.tick().empty());
    CHECK(g.head() == Cell{37, 3});
    CHECK_FALSE(g.queueTurn(Dir::Up));
}

TEST_CASE("hitting the top border going up") {
    Game g(openLevel({5, 0}, Dir::Up), 7);
    parkRabbit(g);
    g.tick();
    CHECK(g.state() == GameState::GameOver);
    CHECK(g.deathCause() == DeathCause::Border);
}

TEST_CASE("hitting a brick is game over") {
    Level lv = openLevel({10, 10}, Dir::Right);
    lv.setTile({12, 10}, Tile::Brick);
    Game g(lv, 7);
    parkRabbit(g);
    g.tick();
    CHECK(g.state() == GameState::Running);
    g.tick();
    CHECK(g.state() == GameState::GameOver);
    CHECK(g.deathCause() == DeathCause::Brick);
    CHECK(g.head() == Cell{11, 10});
}

TEST_CASE("hitting itself is game over") {
    Game g(openLevel({10, 10}, Dir::Right), 7);
    // Grow to length 5.
    REQUIRE(g.placeRabbit({11, 10}));
    g.tick();
    REQUIRE(g.placeRabbit({12, 10}));
    g.tick();
    REQUIRE(g.length() == 5);
    parkRabbit(g);
    // Down, Left, Up: the head runs into the body.
    g.queueTurn(Dir::Down);
    g.tick();
    g.queueTurn(Dir::Left);
    g.tick();
    g.queueTurn(Dir::Up);
    g.tick();
    CHECK(g.state() == GameState::GameOver);
    CHECK(g.deathCause() == DeathCause::Self);
}

TEST_CASE("the head may enter the cell the tail is leaving") {
    Game g(openLevel({10, 10}, Dir::Right), 7);
    REQUIRE(g.placeRabbit({11, 10}));
    g.tick();  // length 4: (11,10) (10,10) (9,10) (8,10)
    parkRabbit(g);
    // A 2x2 loop: Down, Left, Up -> head enters the old tail position as it moves away.
    g.queueTurn(Dir::Down);
    g.tick();  // (11,11) (11,10) (10,10) (9,10)
    g.queueTurn(Dir::Left);
    g.tick();  // (10,11) (11,11) (11,10) (10,10)
    g.queueTurn(Dir::Up);
    g.tick();  // head into (10,10), the tail's cell
    CHECK(g.state() == GameState::Running);
    CHECK(g.head() == Cell{10, 10});
    CHECK(g.length() == 4);
}

TEST_CASE("segments carry in/out directions for straight, turn, head and tail") {
    Game g(openLevel({10, 10}, Dir::Right), 7);
    parkRabbit(g);
    g.queueTurn(Dir::Down);
    g.tick();  // (10,11) head going down, (10,10) turn right->down, (9,10) tail
    auto s = g.segments();
    REQUIRE(s.size() == 3);
    CHECK(s[0].cell == Cell{10, 11});
    CHECK(s[0].in == Dir::Down);
    CHECK(s[0].out == Dir::Down);
    CHECK(s[1].cell == Cell{10, 10});
    CHECK(s[1].in == Dir::Right);
    CHECK(s[1].out == Dir::Down);
    CHECK(s[2].cell == Cell{9, 10});
    CHECK(s[2].in == Dir::Right);
    CHECK(s[2].out == Dir::Right);
}

TEST_CASE("rabbits never spawn on the snake or on bricks") {
    // A crowded level: every second column of bricks, with gaps.
    Level lv;
    for (int x = 1; x < kFieldWidth; x += 2)
        for (int y = 0; y < kFieldHeight - 2; ++y) lv.setTile({x, y}, Tile::Brick);
    lv.setStart({5, 22}, Dir::Right);
    for (std::uint64_t seed = 1; seed <= 200; ++seed) {
        Game g(lv, seed);
        // Grow the snake a few times along the bottom row so it covers more cells.
        for (int i = 0; i < 20 && g.state() == GameState::Running; ++i) {
            Cell ahead = step(g.head(), Dir::Right);
            if (i % 2 == 0) g.placeRabbit(ahead);
            for (const Event& e : g.tick()) {
                if (e.type != EventType::RabbitSpawned) continue;
                CHECK(lv.isFree(e.cell));
                CHECK_FALSE(g.isOnSnake(e.cell));
            }
            if (g.rabbit()) {
                CHECK(lv.isFree(*g.rabbit()));
                CHECK_FALSE(g.isOnSnake(*g.rabbit()));
            }
        }
    }
}

TEST_CASE("the rabbit spawns only on the few free cells left") {
    Level lv;
    lv.fill(Tile::Brick);
    // A corridor of 6 cells: snake takes 3, rabbit must land on one of the other 3.
    for (int x = 0; x < 6; ++x) lv.setTile({x, 0}, Tile::Empty);
    lv.setStart({2, 0}, Dir::Right);
    for (std::uint64_t seed = 1; seed <= 100; ++seed) {
        Game g(lv, seed);
        REQUIRE(g.rabbit());
        CHECK(g.rabbit()->y == 0);
        CHECK(g.rabbit()->x >= 3);
        CHECK(g.rabbit()->x <= 5);
    }
}

TEST_CASE("the same seed gives the same rabbits") {
    Level lv;
    Game a(lv, 42), b(lv, 42), c(lv, 43);
    CHECK(*a.rabbit() == *b.rabbit());
    // Different seeds diverge somewhere within a few spawns.
    bool differs = !(*a.rabbit() == *c.rabbit());
    CHECK(differs);
}

TEST_CASE("level complete after the target number of rabbits") {
    Game g(openLevel({2, 10}, Dir::Right), 7, 3);
    for (int i = 1; i <= 3; ++i) {
        REQUIRE(g.placeRabbit(step(g.head(), Dir::Right)));
        const auto& ev = g.tick();
        CHECK(g.rabbitsEaten() == i);
        if (i < 3) {
            CHECK(g.state() == GameState::Running);
            CHECK_FALSE(hasEvent(ev, EventType::LevelComplete));
        } else {
            CHECK(g.state() == GameState::LevelComplete);
            const Event* lc = findEvent(ev, EventType::LevelComplete);
            REQUIRE(lc);
            CHECK(lc->value == 3);
            CHECK_FALSE(g.rabbit());
            CHECK_FALSE(hasEvent(ev, EventType::RabbitSpawned));
        }
    }
    CHECK(g.length() == 6);
    CHECK(g.tick().empty());
}

TEST_CASE("default target is 50 rabbits") {
    Game g(Level{}, 1);
    CHECK(g.rabbitsToWin() == 50);
    // Feed the snake 50 rabbits in a serpentine over the open field.
    int eaten = 0;
    while (g.state() == GameState::Running && g.ticks() < 5000) {
        Cell h = g.head();
        Dir d = g.heading();
        // Serpentine: run along rows, turn down twice at the ends.
        if (d == Dir::Right && h.x == kFieldWidth - 1) g.queueTurn(Dir::Down);
        else if (d == Dir::Left && h.x == 0) g.queueTurn(Dir::Down);
        else if (d == Dir::Down) g.queueTurn(h.x == 0 ? Dir::Right : Dir::Left);
        Cell next = step(h, g.nextHeading());
        if (eaten < 50) g.placeRabbit(next);
        g.tick();
        eaten = g.rabbitsEaten();
    }
    CHECK(g.state() == GameState::LevelComplete);
    CHECK(g.rabbitsEaten() == 50);
    CHECK(g.length() == kStartLength + 50);
}

TEST_CASE("a start at the edge of the field never puts the body outside") {
    // (5,22) facing up would put the body at (5,23), (5,24): setStart refuses it.
    Level lv;
    CHECK_FALSE(lv.setStart({5, 22}, Dir::Up));
    REQUIRE(lv.setStart(Level::clampStart({5, 22}, Dir::Up), Dir::Up));
    Game g(lv, 3);
    for (Cell c : g.body()) CHECK(insideField(c));
    // Run it up into the border: every tick is in range (the tail is freed inside the field).
    for (int i = 0; i < 40 && g.state() == GameState::Running; ++i) g.tick();
    CHECK(g.state() == GameState::GameOver);
    CHECK(g.deathCause() == DeathCause::Border);
}

TEST_CASE("the level is complete when no free cell is left for a rabbit") {
    Level lv;
    lv.fill(Tile::Brick);
    // A corridor of 6 cells: the snake takes 3, so it can eat only 3 rabbits.
    for (int x = 0; x < 6; ++x) lv.setTile({x, 0}, Tile::Empty);
    REQUIRE(lv.setStart({2, 0}, Dir::Right));
    Game g(lv, 5);
    for (int i = 1; i <= 3; ++i) {
        REQUIRE(g.state() == GameState::Running);
        REQUIRE(g.placeRabbit(step(g.head(), Dir::Right)));
        const auto& ev = g.tick();
        CHECK(g.rabbitsEaten() == i);
        if (i < 3) {
            CHECK_FALSE(hasEvent(ev, EventType::LevelComplete));
            CHECK(g.rabbit());
        } else {
            const Event* lc = findEvent(ev, EventType::LevelComplete);
            REQUIRE(lc);
            CHECK(lc->value == 3);
        }
    }
    CHECK(g.state() == GameState::LevelComplete);
    CHECK(g.rabbitsEaten() < g.rabbitsToWin());
    CHECK_FALSE(g.rabbit());
    CHECK(g.tick().empty());
}

TEST_CASE("a level with no free cell at all is complete right away") {
    Level lv;
    lv.fill(Tile::Brick);
    REQUIRE(lv.setStart({10, 10}, Dir::Right));
    Game g(lv, 1);
    CHECK(g.state() == GameState::LevelComplete);
    CHECK_FALSE(g.rabbit());
    CHECK(hasEvent(g.events(), EventType::LevelComplete));
    CHECK(g.tick().empty());
}
