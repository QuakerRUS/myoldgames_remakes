#include <vs/engine/camera.hpp>

#include <algorithm>
#include <cmath>

namespace vs {

Vec2 Camera2D::view_size() const {
    const float z = zoom > 0 ? zoom : 1.0f;
    return {viewport.w / z, viewport.h / z};
}

RectF Camera2D::view_rect() const {
    const Vec2 size = view_size();
    return {position.x, position.y, size.x, size.y};
}

void Camera2D::center_on(Vec2 target) {
    position = target - view_size() * 0.5f;
    clamp();
}

void Camera2D::follow(Vec2 target, double dt, float stiffness) {
    const Vec2 desired = target - view_size() * 0.5f;
    const float k = static_cast<float>(1.0 - std::exp(-static_cast<double>(stiffness) * dt));
    position += (desired - position) * k;
    clamp();
}

void Camera2D::clamp() {
    if (!bounds_) return;
    const RectF& b = *bounds_;
    const Vec2 size = view_size();
    auto axis = [](float pos, float lo, float world, float view) {
        if (world <= view) return lo - (view - world) * 0.5f;
        return std::clamp(pos, lo, lo + world - view);
    };
    position.x = axis(position.x, b.x, b.w, size.x);
    position.y = axis(position.y, b.y, b.h, size.y);
}

Vec2 Camera2D::world_to_screen(Vec2 world) const {
    Vec2 offset = position * zoom;
    if (pixel_snap) offset = {std::round(offset.x), std::round(offset.y)};
    return {viewport.x + world.x * zoom - offset.x, viewport.y + world.y * zoom - offset.y};
}

Vec2 Camera2D::screen_to_world(Vec2 screen) const {
    Vec2 offset = position * zoom;
    if (pixel_snap) offset = {std::round(offset.x), std::round(offset.y)};
    const float z = zoom > 0 ? zoom : 1.0f;
    return {(screen.x - viewport.x + offset.x) / z, (screen.y - viewport.y + offset.y) / z};
}

}  // namespace vs
