#include <doctest/doctest.h>
#include <vs/engine/error.hpp>
#include <vs/engine/sprite_sheet_data.hpp>

namespace {
const char* kSheet = R"({
  "image": "hero.png",
  "frames": {
    "walk_1": {"x": 32, "y": 0, "w": 32, "h": 32},
    "walk_0": {"x": 0, "y": 0, "w": 32, "h": 32, "pivot_x": 16, "pivot_y": 31}
  },
  "animations": {
    "walk": {"loop": true, "frames": [{"frame": "walk_0", "ms": 120}, {"frame": "walk_1", "ms": 80}]},
    "idle": {"frames": ["walk_0"], "ms": 500, "loop": false}
  }
})";
}  // namespace

TEST_CASE("sprite sheet JSON parses frames and animations in file order") {
    const vs::SpriteSheetData d = vs::parse_sprite_sheet(kSheet);
    CHECK(d.image == "hero.png");
    REQUIRE(d.frames.size() == 2);
    CHECK(d.frames[0].name == "walk_1");
    CHECK(d.frames[1].name == "walk_0");
    const vs::FrameRect* f = d.find_frame("walk_0");
    REQUIRE(f);
    CHECK(f->pivot_x == 16);
    CHECK(f->pivot_y == 31);
    CHECK(d.find_frame("nope") == nullptr);

    REQUIRE(d.animations.size() == 2);
    const vs::AnimationDef* walk = d.find_animation("walk");
    REQUIRE(walk);
    CHECK(walk->loop);
    CHECK(walk->total_ms() == 200);
    const vs::AnimationDef* idle = d.find_animation("idle");
    REQUIRE(idle);
    CHECK_FALSE(idle->loop);
    CHECK(idle->frames[0].ms == 500);
}

TEST_CASE("sprite sheet grid generates frames") {
    const vs::SpriteSheetData d = vs::parse_sprite_sheet(
        R"({"image": "t.png", "grid": {"frame_w": 16, "frame_h": 8, "columns": 3, "count": 5, "prefix": "t", "spacing": 1}})");
    REQUIRE(d.frames.size() == 5);
    CHECK(d.frames[4].name == "t4");
    CHECK(d.frames[4].x == 17);
    CHECK(d.frames[4].y == 9);
}

TEST_CASE("sprite sheet errors") {
    CHECK_THROWS_AS(vs::parse_sprite_sheet("{"), vs::Error);
    CHECK_THROWS_AS(vs::parse_sprite_sheet(R"({"frames": {}})"), vs::Error);  // no image
    CHECK_THROWS_AS(vs::parse_sprite_sheet(R"({"image": "a.png", "frames": {"a": {"x": 0, "y": 0, "w": 0, "h": 1}}})"),
                    vs::Error);
    CHECK_THROWS_AS(vs::parse_sprite_sheet(R"({"image": "a.png", "animations": {"x": {"frames": ["missing"]}}})"),
                    vs::Error);
}

TEST_CASE("sprite sheet serialization round trip") {
    const vs::SpriteSheetData d = vs::parse_sprite_sheet(kSheet);
    const vs::SpriteSheetData e = vs::parse_sprite_sheet(vs::sprite_sheet_to_json_text(d));
    REQUIRE(e.frames.size() == d.frames.size());
    for (std::size_t i = 0; i < d.frames.size(); ++i) {
        CHECK(e.frames[i].name == d.frames[i].name);
        CHECK(e.frames[i].x == d.frames[i].x);
        CHECK(e.frames[i].pivot_y == d.frames[i].pivot_y);
    }
    REQUIRE(e.animations.size() == 2);
    CHECK(e.animations[0].name == "walk");
    CHECK(e.animations[1].frames[0].ms == 500);
    CHECK_FALSE(e.animations[1].loop);
}
