#include "test_util.hpp"

#include <piton/game.hpp>

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <sstream>

using namespace piton;

TEST_CASE("all 10 levels load and pass the checks") {
    for (int n = 1; n <= kLevelCount; ++n) {
        CAPTURE(n);
        Level lv = loadTestLevel(n);
        CHECK_FALSE(lv.name.empty());
        LevelCheck c = checkLevel(lv);
        std::string all;
        for (const auto& p : c.problems) all += p + "; ";
        INFO(all);
        CHECK(c.ok());
        CHECK(c.regions == 1);
        CHECK(c.freeCells >= kMinFreeCells);
        CHECK(c.longestDeadEnd <= kMaxDeadEndLength);
        const int bricks = kCellCount - c.freeCells;
        std::printf("level %02d %-14s bricks %3d free %3d dead-end %d\n", n, lv.name.c_str(), bricks,
                    c.freeCells, c.longestDeadEnd);
    }
}

TEST_CASE("level JSON round trip") {
    Level lv = loadTestLevel(7);
    Level back;
    std::string err;
    REQUIRE(Level::fromJson(lv.toJson(), back, &err));
    CHECK(back == lv);
    CHECK(back.name == "Four Rooms");
}

TEST_CASE("shipped level files are in the canonical toJson() form") {
    for (int n = 1; n <= kLevelCount; ++n) {
        CAPTURE(n);
        std::ifstream in(std::string(PITON_LEVELS_DIR) + "/" + levelFileName(n), std::ios::binary);
        std::ostringstream ss;
        ss << in.rdbuf();
        CHECK(loadTestLevel(n).toJson() == ss.str());
    }
}

TEST_CASE("level JSON errors are reported") {
    Level lv;
    std::string err;
    CHECK_FALSE(Level::fromJson("{", lv, &err));
    CHECK_FALSE(err.empty());
    CHECK_FALSE(Level::fromJson(R"({"format":"other","version":1})", lv, &err));
    std::string json = Level{}.toJson();
    CHECK(Level::fromJson(json, lv, &err));
    std::string bad = json;
    bad.replace(bad.find("\"right\""), 7, "\"north\"");
    CHECK_FALSE(Level::fromJson(bad, lv, &err));
    bad = json;
    bad.replace(bad.find("......"), 1, "x");
    CHECK_FALSE(Level::fromJson(bad, lv, &err));
    CHECK(err.find("unknown character") != std::string::npos);
}

TEST_CASE("level checks catch broken levels") {
    SUBCASE("start on a brick") {
        Level lv;
        lv.setTile(lv.startHead(), Tile::Brick);
        CHECK_FALSE(checkLevel(lv).ok());
    }
    SUBCASE("not enough room ahead") {
        Level lv;
        lv.setStart({33, 5}, Dir::Right);
        CHECK_FALSE(checkLevel(lv).ok());
    }
    SUBCASE("two regions") {
        Level lv;
        for (int y = 0; y < kFieldHeight; ++y) lv.setTile({30, y}, Tile::Brick);
        LevelCheck c = checkLevel(lv);
        CHECK(c.regions == 2);
        CHECK_FALSE(c.ok());
    }
    SUBCASE("long 1-wide dead end") {
        Level lv;
        for (int y = 0; y < 8; ++y) {
            lv.setTile({4, y}, Tile::Brick);
            lv.setTile({6, y}, Tile::Brick);
        }
        LevelCheck c = checkLevel(lv);
        CHECK(c.longestDeadEnd == 8);
        CHECK_FALSE(c.ok());
    }
    SUBCASE("too small") {
        Level lv;
        lv.fill(Tile::Brick);
        for (int x = 0; x < kFieldWidth; ++x) lv.setTile({x, 0}, Tile::Empty);
        lv.setStart({5, 0}, Dir::Right);
        CHECK_FALSE(checkLevel(lv).ok());
    }
    SUBCASE("an empty field is fine") {
        CHECK(checkLevel(Level{}).ok());
    }
}

TEST_CASE("levelFileName") {
    CHECK(levelFileName(1) == "01.json");
    CHECK(levelFileName(10) == "10.json");
}

TEST_CASE("start cells must lie inside the field") {
    CHECK(Level::startFits({5, 20}, Dir::Up));
    CHECK_FALSE(Level::startFits({5, 22}, Dir::Up));   // body at (5,23), (5,24)
    CHECK_FALSE(Level::startFits({5, 0}, Dir::Down));
    CHECK_FALSE(Level::startFits({1, 5}, Dir::Right));
    CHECK_FALSE(Level::startFits({kFieldWidth - 2, 5}, Dir::Left));
    CHECK(Level::startFits({2, 5}, Dir::Right));

    CHECK(Level::clampStart({5, 22}, Dir::Up) == Cell{5, 20});
    CHECK(Level::clampStart({5, 0}, Dir::Down) == Cell{5, 2});
    CHECK(Level::clampStart({0, 4}, Dir::Right) == Cell{2, 4});
    CHECK(Level::clampStart({37, 4}, Dir::Left) == Cell{35, 4});
    CHECK(Level::clampStart({-3, 40}, Dir::Up) == Cell{0, 20});
    CHECK(Level::clampStart({10, 10}, Dir::Left) == Cell{10, 10});
    for (Dir d : kAllDirs)
        for (int y = -2; y < kFieldHeight + 2; ++y)
            for (int x = -2; x < kFieldWidth + 2; ++x) CHECK(Level::startFits(Level::clampStart({x, y}, d), d));

    SUBCASE("setStart refuses a start that does not fit") {
        Level lv;
        const Cell head = lv.startHead();
        const Dir dir = lv.startDir();
        CHECK_FALSE(lv.setStart({5, 22}, Dir::Up));
        CHECK(lv.startHead() == head);
        CHECK(lv.startDir() == dir);
        CHECK(lv.setStart({5, 20}, Dir::Up));
        CHECK(lv.startHead() == Cell{5, 20});
    }
    SUBCASE("setStart clears bricks under the start") {
        Level lv;
        lv.fill(Tile::Brick);
        REQUIRE(lv.setStart({10, 10}, Dir::Left));
        for (Cell c : lv.startCells()) CHECK(lv.isFree(c));
        CHECK(lv.freeCellCount() == kStartLength);
    }
    SUBCASE("fromJson rejects a start sticking out of the field") {
        std::string json = Level{}.toJson();
        json.replace(json.find("\"x\": 20"), 7, "\"x\": 5");
        json.replace(json.find("\"y\": 18"), 7, "\"y\": 22");
        json.replace(json.find("\"right\""), 7, "\"up\"");
        Level lv;
        std::string err;
        CHECK_FALSE(Level::fromJson(json, lv, &err));
        CHECK(err.find("does not fit") != std::string::npos);
    }
    SUBCASE("fromJson repairs bricks under the start") {
        Level brick;
        std::string json = brick.toJson();
        // Row 18: bricks at x = 18..20 (the start cells).
        std::size_t r = json.find("\"rows\": [");
        for (int y = 0; y <= 18; ++y) r = json.find('"', json.find('\n', r) + 1);
        json.replace(r + 1 + 18, 3, "###");
        Level lv;
        std::string err;
        REQUIRE(Level::fromJson(json, lv, &err));
        for (Cell c : lv.startCells()) CHECK(lv.isFree(c));
        CHECK(lv == Level{});
    }
}

TEST_CASE("level check: hard errors and warnings") {
    SUBCASE("an empty field: nothing") {
        LevelCheck c = checkLevel(Level{});
        CHECK(c.playable());
        CHECK(c.errors.empty());
        CHECK(c.warnings.empty());
    }
    SUBCASE("start on a brick is a hard error") {
        Level lv;
        lv.setTile(lv.startCells()[2], Tile::Brick);
        LevelCheck c = checkLevel(lv);
        CHECK_FALSE(c.playable());
        REQUIRE(c.errors.size() == 1);
        CHECK(c.errors[0].find("(18,18)") != std::string::npos);
    }
    SUBCASE("two regions is a hard error") {
        Level lv;
        for (int y = 0; y < kFieldHeight; ++y) lv.setTile({30, y}, Tile::Brick);
        CHECK_FALSE(checkLevel(lv).playable());
    }
    SUBCASE("only the snake fits: hard error") {
        Level lv;
        lv.fill(Tile::Brick);
        REQUIRE(lv.setStart({10, 10}, Dir::Right));
        LevelCheck c = checkLevel(lv);
        CHECK_FALSE(c.playable());
        CHECK(std::find(c.errors.begin(), c.errors.end(), "no free cell for a rabbit") != c.errors.end());
    }
    SUBCASE("small, little room ahead, dead ends: warnings only") {
        Level lv;
        lv.fill(Tile::Brick);
        for (int x = 0; x < kFieldWidth; ++x) lv.setTile({x, 0}, Tile::Empty);
        REQUIRE(lv.setStart({33, 0}, Dir::Right));
        LevelCheck c = checkLevel(lv);
        CHECK(c.playable());
        CHECK_FALSE(c.ok());
        CHECK(c.warnings.size() >= 2);
        CHECK(c.problems.size() == c.errors.size() + c.warnings.size());
    }
    SUBCASE("the shipped levels are playable") {
        for (int n = 1; n <= kLevelCount; ++n) CHECK(checkLevel(loadTestLevel(n)).playable());
    }
}
