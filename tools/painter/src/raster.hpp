#pragma once
// Pixel algorithms on vs::Image (SDL-free): lines, rectangles, flood fill, regions, flips.
#include <algorithm>
#include <utility>
#include <vector>

#include <vs/engine/image.hpp>
#include <vs/engine/math.hpp>

namespace painter {

// Integer rectangle in image pixels. Empty when w <= 0 or h <= 0.
struct IRect {
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;

    bool empty() const { return w <= 0 || h <= 0; }
    int right() const { return x + w; }    // exclusive
    int bottom() const { return y + h; }   // exclusive
    bool contains(int px, int py) const { return px >= x && py >= y && px < x + w && py < y + h; }
    bool operator==(const IRect&) const = default;
};

IRect intersect(const IRect& a, const IRect& b);
IRect unite(const IRect& a, const IRect& b);  // bounding box; an empty operand is ignored
IRect bounds(const vs::Image& image);
// Rectangle spanned by two corner pixels (both inclusive, any order).
IRect span(int x0, int y0, int x1, int y1);

// Pixels of a Bresenham line from (x0,y0) to (x1,y1), both ends included.
std::vector<std::pair<int, int>> line_points(int x0, int y0, int x1, int y1);

// Drawing primitives write `c` (replacing, not blending) and only inside `clip`.
void plot(vs::Image& img, int x, int y, vs::Color c, const IRect& clip);
void draw_line(vs::Image& img, int x0, int y0, int x1, int y1, vs::Color c, const IRect& clip);
void draw_rect(vs::Image& img, const IRect& r, vs::Color c, bool filled, const IRect& clip);
// 4-way flood fill of the area of the exact colour at (x, y). Returns the number of pixels changed.
int flood_fill(vs::Image& img, int x, int y, vs::Color c, const IRect& clip);

// Copy of a region (clipped to the image).
vs::Image copy_region(const vs::Image& img, const IRect& r);
// Writes `src` with its top-left corner at (x, y). skip_transparent: pixels with alpha 0 leave
// the destination unchanged (pasting a sprite over a background).
void blit(vs::Image& dst, const vs::Image& src, int x, int y, const IRect& clip, bool skip_transparent = false);

void flip_horizontal(vs::Image& img, const IRect& r);
void flip_vertical(vs::Image& img, const IRect& r);

// Bounding box of the pixels that differ between two images of the same size.
IRect diff_bounds(const vs::Image& a, const vs::Image& b);
// Image of a new size; the old content stays at the top-left, new pixels are transparent.
vs::Image resized_canvas(const vs::Image& img, int w, int h);

}  // namespace painter
