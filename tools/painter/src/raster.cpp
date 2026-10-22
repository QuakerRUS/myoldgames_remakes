#include "raster.hpp"

#include <cstdlib>
#include <cstring>

namespace painter {

IRect intersect(const IRect& a, const IRect& b) {
    const int x0 = std::max(a.x, b.x);
    const int y0 = std::max(a.y, b.y);
    const int x1 = std::min(a.right(), b.right());
    const int y1 = std::min(a.bottom(), b.bottom());
    if (x1 <= x0 || y1 <= y0) return {};
    return {x0, y0, x1 - x0, y1 - y0};
}

IRect unite(const IRect& a, const IRect& b) {
    if (a.empty()) return b;
    if (b.empty()) return a;
    const int x0 = std::min(a.x, b.x);
    const int y0 = std::min(a.y, b.y);
    return {x0, y0, std::max(a.right(), b.right()) - x0, std::max(a.bottom(), b.bottom()) - y0};
}

IRect bounds(const vs::Image& image) { return {0, 0, image.width, image.height}; }

IRect span(int x0, int y0, int x1, int y1) {
    return {std::min(x0, x1), std::min(y0, y1), std::abs(x1 - x0) + 1, std::abs(y1 - y0) + 1};
}

std::vector<std::pair<int, int>> line_points(int x0, int y0, int x1, int y1) {
    std::vector<std::pair<int, int>> out;
    const int dx = std::abs(x1 - x0);
    const int dy = -std::abs(y1 - y0);
    const int sx = x0 < x1 ? 1 : -1;
    const int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    for (;;) {
        out.emplace_back(x0, y0);
        if (x0 == x1 && y0 == y1) break;
        const int e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
    return out;
}

void plot(vs::Image& img, int x, int y, vs::Color c, const IRect& clip) {
    if (img.in_bounds(x, y) && clip.contains(x, y)) img.set(x, y, c);
}

void draw_line(vs::Image& img, int x0, int y0, int x1, int y1, vs::Color c, const IRect& clip) {
    for (const auto& [x, y] : line_points(x0, y0, x1, y1)) plot(img, x, y, c, clip);
}

void draw_rect(vs::Image& img, const IRect& r, vs::Color c, bool filled, const IRect& clip) {
    if (r.empty()) return;
    const IRect area = intersect(intersect(r, clip), bounds(img));
    for (int y = area.y; y < area.bottom(); ++y)
        for (int x = area.x; x < area.right(); ++x) {
            const bool edge = x == r.x || y == r.y || x == r.right() - 1 || y == r.bottom() - 1;
            if (filled || edge) img.set(x, y, c);
        }
}

int flood_fill(vs::Image& img, int x, int y, vs::Color c, const IRect& clip) {
    const IRect area = intersect(clip, bounds(img));
    if (!area.contains(x, y)) return 0;
    const vs::Color target = img.get(x, y);
    if (target == c) return 0;
    int changed = 0;
    std::vector<std::pair<int, int>> stack{{x, y}};
    while (!stack.empty()) {
        auto [px, py] = stack.back();
        stack.pop_back();
        if (!area.contains(px, py) || img.get(px, py) != target) continue;
        // Scanline: extend left and right, then queue the rows above and below.
        int left = px;
        while (left - 1 >= area.x && img.get(left - 1, py) == target) --left;
        int right = px;
        while (right + 1 < area.right() && img.get(right + 1, py) == target) ++right;
        for (int i = left; i <= right; ++i) {
            img.set(i, py, c);
            ++changed;
            if (py > area.y && img.get(i, py - 1) == target) stack.emplace_back(i, py - 1);
            if (py + 1 < area.bottom() && img.get(i, py + 1) == target) stack.emplace_back(i, py + 1);
        }
    }
    return changed;
}

vs::Image copy_region(const vs::Image& img, const IRect& r) {
    const IRect a = intersect(r, bounds(img));
    vs::Image out(a.w, a.h);
    for (int y = 0; y < a.h; ++y)
        std::memcpy(&out.pixels[static_cast<std::size_t>(y) * a.w * 4],
                    &img.pixels[(static_cast<std::size_t>(a.y + y) * img.width + a.x) * 4], static_cast<std::size_t>(a.w) * 4);
    return out;
}

void blit(vs::Image& dst, const vs::Image& src, int x, int y, const IRect& clip, bool skip_transparent) {
    const IRect area = intersect(intersect(IRect{x, y, src.width, src.height}, clip), bounds(dst));
    for (int py = area.y; py < area.bottom(); ++py)
        for (int px = area.x; px < area.right(); ++px) {
            const vs::Color c = src.get(px - x, py - y);
            if (skip_transparent && c.a == 0) continue;
            dst.set(px, py, c);
        }
}

void flip_horizontal(vs::Image& img, const IRect& r) {
    const IRect a = intersect(r, bounds(img));
    for (int y = a.y; y < a.bottom(); ++y)
        for (int i = 0; i < a.w / 2; ++i) {
            const vs::Color l = img.get(a.x + i, y);
            img.set(a.x + i, y, img.get(a.right() - 1 - i, y));
            img.set(a.right() - 1 - i, y, l);
        }
}

void flip_vertical(vs::Image& img, const IRect& r) {
    const IRect a = intersect(r, bounds(img));
    for (int i = 0; i < a.h / 2; ++i)
        for (int x = a.x; x < a.right(); ++x) {
            const vs::Color t = img.get(x, a.y + i);
            img.set(x, a.y + i, img.get(x, a.bottom() - 1 - i));
            img.set(x, a.bottom() - 1 - i, t);
        }
}

IRect diff_bounds(const vs::Image& a, const vs::Image& b) {
    if (a.width != b.width || a.height != b.height) return bounds(b);
    int x0 = a.width, y0 = a.height, x1 = -1, y1 = -1;
    for (int y = 0; y < a.height; ++y) {
        const std::size_t row = static_cast<std::size_t>(y) * a.width * 4;
        if (std::memcmp(&a.pixels[row], &b.pixels[row], static_cast<std::size_t>(a.width) * 4) == 0) continue;
        for (int x = 0; x < a.width; ++x)
            if (std::memcmp(&a.pixels[row + static_cast<std::size_t>(x) * 4], &b.pixels[row + static_cast<std::size_t>(x) * 4], 4) != 0) {
                x0 = std::min(x0, x);
                x1 = std::max(x1, x);
                y0 = std::min(y0, y);
                y1 = std::max(y1, y);
            }
    }
    if (x1 < 0) return {};
    return {x0, y0, x1 - x0 + 1, y1 - y0 + 1};
}

vs::Image resized_canvas(const vs::Image& img, int w, int h) {
    vs::Image out(w, h);
    blit(out, img, 0, 0, bounds(out));
    return out;
}

}  // namespace painter
