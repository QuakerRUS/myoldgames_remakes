#pragma once
#include <optional>

#include <vs/engine/math.hpp>

namespace vs {

// 2D camera: maps world coordinates to logical screen coordinates.
//   screen = viewport.xy + (world - position) * zoom
// `position` is the world point shown at the top-left corner of the viewport.
class Camera2D {
public:
    RectF viewport{0, 0, 1280, 720};  // screen area the camera draws into (logical px)
    Vec2 position{};                  // world point at the viewport's top-left corner
    float zoom = 1.0f;                // screen px per world unit (use integers for pixel art)
    bool pixel_snap = true;           // world_to_screen rounds the scroll offset to whole screen px

    // World-space size of the visible area (viewport size / zoom).
    Vec2 view_size() const;
    // Visible world rectangle.
    RectF view_rect() const;

    void set_bounds(RectF world) { bounds_ = world; }
    void clear_bounds() { bounds_.reset(); }
    const std::optional<RectF>& bounds() const { return bounds_; }

    // Moves the camera immediately so that `target` is at the viewport centre, then clamps.
    void center_on(Vec2 target);
    // Smooth follow: moves towards centring `target` with exponential smoothing,
    //   position += (desired - position) * (1 - exp(-stiffness * dt)),
    // then clamps. Larger stiffness = snappier. Call from update() with the fixed dt.
    void follow(Vec2 target, double dt, float stiffness = 8.0f);
    // Keeps the view inside the bounds. On an axis where the world is smaller than the
    // view, the world is centred in the view instead. No-op without bounds.
    void clamp();

    Vec2 world_to_screen(Vec2 world) const;
    Vec2 screen_to_world(Vec2 screen) const;

private:
    std::optional<RectF> bounds_;
};

}  // namespace vs
