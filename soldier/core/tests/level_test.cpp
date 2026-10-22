#include "util.hpp"

using namespace soldier;
using soldier::test::make_level;

TEST_CASE("level JSON parse and write") {
    const char* text = R"({"number": 7, "width": 3, "height": 2, "time_limit": 15,
                           "rows": ["#v#", "~X%"]})";
    const Level lv = parse_level_json(text);
    CHECK(lv.number == 7);
    CHECK(lv.width == 3);
    CHECK(lv.height == 2);
    CHECK(lv.time_limit == 15);
    CHECK(lv.at({1, 0}) == Tile::PlayerDown);
    CHECK(lv.at({0, 1}) == Tile::Water);
    CHECK(lv.at({2, 1}) == Tile::DarkWall);
    const Level again = parse_level_json(level_to_json(lv));
    CHECK(again.cells == lv.cells);
    CHECK(again.time_limit == 15);
}

TEST_CASE("level JSON errors") {
    CHECK_THROWS_AS(parse_level_json("{"), FormatError);
    CHECK_THROWS_AS(parse_level_json(R"({"number":1,"width":2,"height":1,"time_limit":0,"rows":["v"]})"), FormatError);
    CHECK_THROWS_AS(parse_level_json(R"({"number":1,"width":1,"height":1,"time_limit":0,"rows":["Z"]})"), FormatError);
    CHECK_THROWS_AS(parse_level_json(R"({"number":1,"width":1,"height":1,"time_limit":0,"rows":["."]})"), FormatError);
    CHECK_THROWS_AS(parse_level_json(R"({"number":1,"width":1,"height":1,"rows":["v"]})"), FormatError);
}

TEST_CASE("start is the last player tile in row-major order; sunk-box players are ignored") {
    const Level lv = make_level({"v.>", "(.."});
    CHECK(lv.start() == Point{2, 0});
    Game g(lv);
    CHECK(g.player() == Point{2, 0});
    CHECK(g.facing() == Dir::Right);
    CHECK(g.cell({0, 0}) == Tile::PlayerDown);  // decoration stays
}

TEST_CASE("player may start on a button, in a door or under a guillotine") {
    Game a(make_level({"_3"}));
    CHECK(a.ground() == Ground::Button);
    CHECK(a.facing() == Dir::Left);
    CHECK(a.unpressed_buttons() == 1);  // only free buttons (code 9) are counted
    Game b(make_level({"6"}));
    CHECK(b.ground() == Ground::Door);
    Game c(make_level({"}"}));
    CHECK(c.ground() == Ground::Guillotine);
}

TEST_CASE("all 51 imported levels load") {
    for (int n = 1; n <= 51; ++n) {
        CAPTURE(n);
        const Level lv = load_level_file(test::data_path("levels/" + test::two_digits(n) + ".json"));
        CHECK(lv.number == n);
        const bool timed = n == 1 || n == 24 || n == 33 || n == 34;
        CHECK((lv.time_limit != 0) == timed);
        Game g(lv);
        CHECK(g.status() == Status::Playing);
    }
    CHECK(load_level_file(test::data_path("levels/01.json")).time_limit == 300);
    CHECK(load_level_file(test::data_path("levels/24.json")).time_limit == 120);
    CHECK(load_level_file(test::data_path("levels/33.json")).time_limit == 36);
    CHECK(load_level_file(test::data_path("levels/34.json")).time_limit == 15);
}
