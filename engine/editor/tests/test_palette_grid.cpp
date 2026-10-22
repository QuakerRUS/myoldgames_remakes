#include <doctest/doctest.h>
#include <vs/editor/palette_grid.hpp>

TEST_CASE("palette layout: every group starts a new row") {
    const auto cells = vs::palette_layout({3, 5}, 4);
    REQUIRE(cells.size() == 8);
    CHECK(cells[2] == vs::PaletteCell{0, 2});
    CHECK(cells[3] == vs::PaletteCell{1, 0});
    CHECK(cells[7] == vs::PaletteCell{2, 0});
}

TEST_CASE("palette navigation: wrapping and clamping") {
    const std::vector<int> groups{3, 5};
    CHECK(vs::palette_move(groups, 4, 0, -1, 0, true) == 7);
    CHECK(vs::palette_move(groups, 4, 7, 1, 0, true) == 0);
    CHECK(vs::palette_move(groups, 4, 2, 0, 1, true) == 5);   // row 1, nearest column to 2
    CHECK(vs::palette_move(groups, 4, 0, 0, -1, true) == 7);  // up from the first row: last row
    CHECK(vs::palette_move(groups, 4, 0, -1, 0, false) == 0);
    CHECK(vs::palette_move(groups, 4, 1, 0, 1, false) == 5);
    CHECK(vs::palette_move(groups, 4, 6, 0, 1, false) == 7);
}
