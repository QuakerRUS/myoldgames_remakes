#include <doctest/doctest.h>

#include <vs/engine/error.hpp>

#include "palette.hpp"

using namespace painter;

TEST_CASE("hex colours") {
    CHECK(to_hex({255, 0, 16, 255}) == "FF0010");
    CHECK(to_hex({255, 0, 16, 128}) == "FF001080");
    CHECK(parse_hex("#ff0010") == vs::Color{255, 0, 16, 255});
    CHECK(parse_hex("FF001080") == vs::Color{255, 0, 16, 128});
    CHECK_FALSE(parse_hex("12345").has_value());
    CHECK_FALSE(parse_hex("GG0000").has_value());
}

TEST_CASE("palette from image: unique, transparent first, greys before colours") {
    vs::Image img(4, 1);
    img.set(0, 0, {200, 30, 30, 255});
    img.set(1, 0, {255, 255, 255, 255});
    img.set(2, 0, {10, 20, 30, 0});  // transparent: counts as {0,0,0,0}
    img.set(3, 0, {0, 0, 0, 255});
    const Palette p = palette_from_image(img);
    REQUIRE(p.size() == 4);
    CHECK(p[0] == vs::Color{0, 0, 0, 0});
    CHECK(p[1] == vs::Color{0, 0, 0, 255});
    CHECK(p[2] == vs::Color{255, 255, 255, 255});
    CHECK(p[3] == vs::Color{200, 30, 30, 255});
    CHECK(palette_from_image(img, 2).size() == 2);
}

TEST_CASE(".hex and .gpl round trip") {
    const Palette p = {{0, 0, 0, 0}, {1, 2, 3, 255}, {250, 128, 7, 255}, {9, 9, 9, 100}};
    CHECK(parse_hex_palette(hex_palette_text(p)) == p);
    const Palette g = parse_gpl_palette(gpl_palette_text(p, "test"));
    REQUIRE(g.size() == 3);  // no alpha in GPL: the transparent entry is dropped
    CHECK(g[0] == vs::Color{1, 2, 3, 255});
    CHECK(g[2] == vs::Color{9, 9, 9, 255});
    CHECK(parse_gpl_palette("GIMP Palette\nName: x\nColumns: 4\n# comment\n  1   2   3 Untitled\n").size() == 1);
    CHECK_THROWS_AS(parse_gpl_palette("not a palette"), vs::Error);
    CHECK_THROWS_AS(parse_hex_palette("zzz\n"), vs::Error);
}
