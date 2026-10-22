#include <doctest/doctest.h>

#include "document.hpp"
#include "sheet_ops.hpp"

using namespace painter;

namespace {
const char* kJson = R"({
  "image": "a.png",
  "note": {"by": "another tool"},
  "grid": {"frame_w": 8, "frame_h": 8, "columns": 2, "count": 2, "prefix": "g", "spacing": 1},
  "frames": {"hero": {"x": 0, "y": 9, "w": 8, "h": 12, "pivot_x": 0, "pivot_y": 4}},
  "animations": {"walk": {"ms": 120, "frames": ["g0", {"frame": "g1", "ms": 60}]},
                 "once": {"loop": false, "frames": ["hero"]}}
})";
}  // namespace

TEST_CASE("sheet parse keeps extra keys, expands the grid, writes canonical JSON") {
    const Sheet s = parse_sheet(kJson);
    REQUIRE(s.data.frames.size() == 3);
    CHECK(s.data.frames[1].name == "g1");
    CHECK(s.data.frames[1].x == 9);
    CHECK(s.extra.contains("note"));
    CHECK_FALSE(s.extra.contains("grid"));
    const std::string text = sheet_to_json_text(s);
    CHECK(text.find("\"grid\"") == std::string::npos);
    CHECK(text.find("another tool") != std::string::npos);
    const Sheet back = parse_sheet(text);
    std::string why;
    CHECK(sheets_equivalent(s, back, &why));
    CHECK(sheet_to_json_text(back) == text);  // stable
    CHECK(validate_sheet(back, 17, 21).empty());
    CHECK_FALSE(validate_sheet(back, 16, 16).empty());  // frames outside
}

TEST_CASE("rename and delete frames keep animations consistent") {
    Sheet s = parse_sheet(kJson);
    CHECK(rename_frame(s, 0, "first"));
    CHECK(s.data.animations[0].frames[0].frame == "first");
    CHECK_FALSE(rename_frame(s, 0, "g1"));  // taken
    CHECK_FALSE(rename_frame(s, 0, ""));
    CHECK(delete_frame(s, 2) == 1);  // "once" became empty and is removed
    CHECK(s.data.animations.size() == 1);
    CHECK(unique_frame_name(s, "g1") == "g1_2");
    CHECK(unique_animation_name(s, "walk") == "walk_2");
}

TEST_CASE("tile grid helpers") {
    const Sheet s = parse_sheet(kJson);
    const TileGrid g = guess_tile_grid(s, {});
    CHECK(g.w == 8);
    CHECK(g.gap == 1);
    CHECK(snap_to_tiles({10, 1, 3, 3}, g) == IRect{9, 0, 8, 8});
    CHECK(snap_to_tiles({2, 2, 10, 2}, g) == IRect{0, 0, 17, 8});
    const std::optional<IRect> slot = find_free_slot(s, 40, 40, 8, 8, g);
    REQUIRE(slot.has_value());
    CHECK(*slot == IRect{18, 0, 8, 8});
    CHECK_FALSE(find_free_slot(s, 16, 16, 8, 8, g).has_value());
}

TEST_CASE("animation timing and onion-skin predecessor") {
    const Sheet s = parse_sheet(kJson);
    const vs::AnimationDef& walk = s.data.animations[0];
    CHECK(animation_entry_at(walk, 0) == 0);
    CHECK(animation_entry_at(walk, 119) == 0);
    CHECK(animation_entry_at(walk, 121) == 1);
    CHECK(animation_entry_at(walk, 181) == 0);  // loops at 180
    const vs::AnimationDef& once = s.data.animations[1];
    CHECK(animation_entry_at(once, 10000) == 0);
    CHECK(previous_frame_index(s, 0, 0) == 1);  // walk loops: before g0 comes g1
    CHECK(previous_frame_index(s, 1, 0) == 0);
    CHECK(previous_frame_index(s, 2, -1) == 1);
}
