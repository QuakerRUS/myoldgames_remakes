#include <doctest/doctest.h>

#include "commands.hpp"
#include "raster.hpp"

using namespace painter;

namespace {
const vs::Color R{255, 0, 0, 255};
const vs::Color G{0, 255, 0, 255};
const vs::Color T{0, 0, 0, 0};
}  // namespace

TEST_CASE("line points include both ends and have no gaps") {
    const auto pts = line_points(0, 0, 5, 2);
    CHECK(pts.front() == std::pair{0, 0});
    CHECK(pts.back() == std::pair{5, 2});
    CHECK(pts.size() == 6);
    CHECK(line_points(3, 3, 3, 3).size() == 1);
}

TEST_CASE("flood fill is 4-way, exact colour, clipped") {
    vs::Image img(5, 5, T);
    // A diagonal wall: 4-way fill must not leak through the diagonal.
    for (int i = 0; i < 5; ++i) img.set(i, 4 - i, G);
    const int n = flood_fill(img, 0, 0, R, bounds(img));
    CHECK(n == 10);  // the triangle above the anti-diagonal
    CHECK(img.get(4, 4) == T);
    CHECK(img.get(0, 0) == R);
    CHECK(flood_fill(img, 0, 0, R, bounds(img)) == 0);  // same colour: nothing
    vs::Image img2(4, 4, T);
    CHECK(flood_fill(img2, 0, 0, R, {0, 0, 2, 2}) == 4);
    CHECK(img2.get(2, 2) == T);
}

TEST_CASE("rectangles: outline and filled") {
    vs::Image img(5, 5, T);
    draw_rect(img, {0, 0, 5, 5}, R, false, bounds(img));
    CHECK(img.get(0, 0) == R);
    CHECK(img.get(4, 4) == R);
    CHECK(img.get(2, 2) == T);
    draw_rect(img, {1, 1, 3, 3}, G, true, bounds(img));
    CHECK(img.get(2, 2) == G);
    CHECK(span(3, 4, 1, 1) == IRect{1, 1, 3, 4});
}

TEST_CASE("flip, copy, blit and diff") {
    vs::Image img(3, 2, T);
    img.set(0, 0, R);
    vs::Image orig = img;
    flip_horizontal(img, bounds(img));
    CHECK(img.get(2, 0) == R);
    flip_vertical(img, bounds(img));
    CHECK(img.get(2, 1) == R);
    CHECK(diff_bounds(orig, img) == IRect{0, 0, 3, 2});
    const vs::Image part = copy_region(img, {1, 1, 2, 1});
    CHECK(part.width == 2);
    CHECK(part.get(1, 0) == R);
    vs::Image dst(3, 2, G);
    blit(dst, part, 0, 0, bounds(dst), true);  // transparent pixel skipped
    CHECK(dst.get(0, 0) == G);
    CHECK(dst.get(1, 0) == R);
    CHECK(diff_bounds(dst, dst).empty());
}

TEST_CASE("region command undo/redo restores pixels") {
    Document doc;
    doc.image = vs::Image(4, 4, T);
    vs::UndoStack undo;
    const vs::Image before = doc.image;
    vs::Image after = before;
    draw_rect(after, {1, 1, 2, 2}, R, true, bounds(after));
    const IRect r = diff_bounds(before, after);
    undo.push(std::make_unique<RegionCommand>(doc, r, copy_region(before, r), copy_region(after, r), "Paint"));
    CHECK(doc.image.pixels == after.pixels);
    undo.undo();
    CHECK(doc.image.pixels == before.pixels);
    undo.redo();
    CHECK(doc.image.pixels == after.pixels);
    CHECK(resized_canvas(after, 6, 2).get(1, 1) == R);
}
