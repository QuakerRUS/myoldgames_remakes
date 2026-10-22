#pragma once
#include <optional>
#include <vector>

#include <vs/engine/math.hpp>

struct SDL_Renderer;

namespace vs {

class Texture;

enum class Flip { None = 0, Horizontal = 1, Vertical = 2, Both = 3 };

// Options for drawing a texture region / sprite frame.
struct DrawParams {
    Vec2 scale{1.0f, 1.0f};
    Flip flip = Flip::None;
    float rotation = 0.0f;              // degrees, clockwise, around the origin
    std::optional<Vec2> origin;         // pivot in source pixels; default: frame pivot (sprites) or (0,0)
    Color tint{255, 255, 255, 255};     // colour modulation; tint.a = alpha
};

// Thin drawing wrapper around SDL_Renderer. All coordinates are logical pixels
// (1280x720 by default, see AppConfig). Owned by App; scenes receive it in render().
class Renderer {
public:
    explicit Renderer(SDL_Renderer* renderer) : renderer_(renderer) {}
    SDL_Renderer* sdl() const { return renderer_; }

    // Logical size (the 1280x720 canvas, or the window's pixel size without logical presentation).
    Vec2 size() const;

    void clear(Color color);

    // --- primitives (alpha-blended) ---
    void fill_rect(RectF rect, Color color);
    void draw_rect(RectF rect, Color color, float thickness = 1.0f);  // outline inside the rect
    void draw_line(Vec2 a, Vec2 b, Color color, float thickness = 1.0f);
    void fill_rounded_rect(RectF rect, float radius, Color color);
    void draw_rounded_rect(RectF rect, float radius, Color color, float thickness = 1.0f);
    void fill_circle(Vec2 center, float radius, Color color);
    // Vertical gradient fill (top colour to bottom colour), optionally rounded.
    void fill_gradient(RectF rect, Color top, Color bottom, float radius = 0.0f);

    // --- textures ---
    // Draws `src` (texture pixels; nullopt = whole texture) so that the origin point lands on
    // `pos`; size = src size * scale.
    void draw(const Texture& texture, std::optional<RectF> src, Vec2 pos, const DrawParams& params = {});
    // Stretches `src` into `dst` (no rotation).
    void draw_stretched(const Texture& texture, std::optional<RectF> src, RectF dst, Color tint = {});

    // Clipping in logical coordinates; nullopt disables it.
    void set_clip(std::optional<RectF> rect);

private:
    void fill_polygon_fan(const std::vector<Vec2>& points, Color color);

    SDL_Renderer* renderer_;
};

// Points of a rounded rectangle's outline, clockwise from the top-left arc
// (`segments` points per corner). SDL-free helper used by the rounded primitives.
std::vector<Vec2> rounded_rect_outline(RectF rect, float radius, int segments = 8);

}  // namespace vs
