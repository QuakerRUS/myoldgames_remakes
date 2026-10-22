#include "sheet_ops.hpp"

#include <algorithm>
#include <cmath>

namespace painter {

namespace {

int floor_div(int a, int b) { return a >= 0 ? a / b : -((-a + b - 1) / b); }

template <class Exists>
std::string unique_name(std::string_view base, Exists exists) {
    std::string b(base.empty() ? std::string_view("frame") : base);
    if (!exists(b)) return b;
    for (int i = 2;; ++i) {
        std::string candidate = b + "_" + std::to_string(i);
        if (!exists(candidate)) return candidate;
    }
}

}  // namespace

TileGrid guess_tile_grid(const Sheet& sheet, TileGrid fallback) {
    const auto& frames = sheet.data.frames;
    if (frames.empty()) return fallback;
    TileGrid g{frames[0].w, frames[0].h, 0};
    for (const vs::FrameRect& f : frames)
        if ((f.y == frames[0].y && f.x == frames[0].x + frames[0].w + 1) || (f.x == frames[0].x && f.y == frames[0].y + frames[0].h + 1)) {
            g.gap = 1;
            break;
        }
    return g;
}

IRect snap_to_tiles(const IRect& r, const TileGrid& g) {
    if (g.w <= 0 || g.h <= 0) return r;
    const int c0 = floor_div(r.x, g.pitch_x());
    const int r0 = floor_div(r.y, g.pitch_y());
    const int c1 = floor_div(r.right() - 1, g.pitch_x());
    const int r1 = floor_div(r.bottom() - 1, g.pitch_y());
    return {c0 * g.pitch_x(), r0 * g.pitch_y(), (c1 - c0) * g.pitch_x() + g.w, (r1 - r0) * g.pitch_y() + g.h};
}

std::string unique_frame_name(const Sheet& sheet, std::string_view base) {
    return unique_name(base, [&](const std::string& n) { return sheet.data.find_frame(n) != nullptr; });
}

std::string unique_animation_name(const Sheet& sheet, std::string_view base) {
    return unique_name(base, [&](const std::string& n) { return sheet.data.find_animation(n) != nullptr; });
}

IRect frame_rect(const vs::FrameRect& f) { return {f.x, f.y, f.w, f.h}; }

std::optional<IRect> find_free_slot(const Sheet& sheet, int image_w, int image_h, int w, int h, const TileGrid& grid) {
    const int px = std::max(grid.pitch_x(), 1);
    const int py = std::max(grid.pitch_y(), 1);
    for (int y = 0; y + h <= image_h; y += py)
        for (int x = 0; x + w <= image_w; x += px) {
            const IRect cand{x, y, w, h};
            const bool overlaps = std::any_of(sheet.data.frames.begin(), sheet.data.frames.end(),
                                              [&](const vs::FrameRect& f) { return !intersect(cand, frame_rect(f)).empty(); });
            if (!overlaps) return cand;
        }
    return std::nullopt;
}

bool rename_frame(Sheet& sheet, std::size_t index, const std::string& name) {
    if (index >= sheet.data.frames.size() || name.empty()) return false;
    std::string& current = sheet.data.frames[index].name;
    if (current == name) return true;
    if (sheet.data.find_frame(name)) return false;
    for (vs::AnimationDef& a : sheet.data.animations)
        for (vs::AnimationFrame& f : a.frames)
            if (f.frame == current) f.frame = name;
    current = name;
    return true;
}

bool rename_animation(Sheet& sheet, std::size_t index, const std::string& name) {
    if (index >= sheet.data.animations.size() || name.empty()) return false;
    if (sheet.data.animations[index].name == name) return true;
    if (sheet.data.find_animation(name)) return false;
    sheet.data.animations[index].name = name;
    return true;
}

int delete_frame(Sheet& sheet, std::size_t index) {
    if (index >= sheet.data.frames.size()) return 0;
    const std::string name = sheet.data.frames[index].name;
    sheet.data.frames.erase(sheet.data.frames.begin() + static_cast<std::ptrdiff_t>(index));
    for (vs::AnimationDef& a : sheet.data.animations)
        std::erase_if(a.frames, [&](const vs::AnimationFrame& f) { return f.frame == name; });
    return static_cast<int>(std::erase_if(sheet.data.animations, [](const vs::AnimationDef& a) { return a.frames.empty(); }));
}

int previous_frame_index(const Sheet& sheet, int frame, int anim) {
    const auto& frames = sheet.data.frames;
    if (frame < 0 || frame >= static_cast<int>(frames.size())) return -1;
    if (anim >= 0 && anim < static_cast<int>(sheet.data.animations.size())) {
        const vs::AnimationDef& a = sheet.data.animations[static_cast<std::size_t>(anim)];
        for (std::size_t k = 0; k < a.frames.size(); ++k)
            if (a.frames[k].frame == frames[static_cast<std::size_t>(frame)].name) {
                if (k == 0 && !a.loop) break;
                const std::string& prev = a.frames[k == 0 ? a.frames.size() - 1 : k - 1].frame;
                if (prev == frames[static_cast<std::size_t>(frame)].name) break;
                for (std::size_t i = 0; i < frames.size(); ++i)
                    if (frames[i].name == prev) return static_cast<int>(i);
                break;
            }
    }
    return frame - 1;
}

std::size_t animation_entry_at(const vs::AnimationDef& anim, double elapsed_ms) {
    const int total = anim.total_ms();
    if (anim.frames.empty() || total <= 0) return 0;
    double t = elapsed_ms;
    if (anim.loop)
        t = std::fmod(std::max(t, 0.0), static_cast<double>(total));
    else if (t >= total)
        return anim.frames.size() - 1;
    for (std::size_t i = 0; i < anim.frames.size(); ++i) {
        t -= anim.frames[i].ms;
        if (t < 0) return i;
    }
    return anim.frames.size() - 1;
}

}  // namespace painter
