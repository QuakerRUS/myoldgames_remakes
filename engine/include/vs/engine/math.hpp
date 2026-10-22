#pragma once
#include <cstdint>

namespace vs {

// 2D vector in logical pixels or world units.
struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;

    constexpr Vec2 operator+(Vec2 o) const { return {x + o.x, y + o.y}; }
    constexpr Vec2 operator-(Vec2 o) const { return {x - o.x, y - o.y}; }
    constexpr Vec2 operator*(float s) const { return {x * s, y * s}; }
    constexpr Vec2 operator/(float s) const { return {x / s, y / s}; }
    constexpr Vec2& operator+=(Vec2 o) { x += o.x; y += o.y; return *this; }
    constexpr Vec2& operator-=(Vec2 o) { x -= o.x; y -= o.y; return *this; }
    constexpr bool operator==(const Vec2&) const = default;
};

// Linear interpolation; used with the render alpha of App: lerp(prev, cur, alpha).
constexpr float lerp(float a, float b, float t) { return a + (b - a) * t; }
constexpr Vec2 lerp(Vec2 a, Vec2 b, float t) { return {lerp(a.x, b.x, t), lerp(a.y, b.y, t)}; }

// Axis-aligned rectangle: top-left corner plus size.
struct RectF {
    float x = 0.0f;
    float y = 0.0f;
    float w = 0.0f;
    float h = 0.0f;

    constexpr float right() const { return x + w; }
    constexpr float bottom() const { return y + h; }
    constexpr Vec2 position() const { return {x, y}; }
    constexpr Vec2 size() const { return {w, h}; }
    constexpr Vec2 center() const { return {x + w * 0.5f, y + h * 0.5f}; }
    constexpr bool contains(Vec2 p) const { return p.x >= x && p.y >= y && p.x < x + w && p.y < y + h; }
    // Rectangle shrunk by d on every side (grown when d is negative).
    constexpr RectF inset(float d) const { return {x + d, y + d, w - 2 * d, h - 2 * d}; }
    constexpr bool operator==(const RectF&) const = default;
};

// 8-bit RGBA colour.
struct Color {
    std::uint8_t r = 255;
    std::uint8_t g = 255;
    std::uint8_t b = 255;
    std::uint8_t a = 255;

    constexpr bool operator==(const Color&) const = default;
    // Same colour with another alpha.
    constexpr Color with_alpha(std::uint8_t alpha) const { return {r, g, b, alpha}; }
};

// Colour from 0xRRGGBB (opaque) — e.g. rgb(0x1E2430).
constexpr Color rgb(std::uint32_t hex) {
    return {static_cast<std::uint8_t>(hex >> 16), static_cast<std::uint8_t>(hex >> 8), static_cast<std::uint8_t>(hex), 255};
}
// Colour from 0xRRGGBBAA.
constexpr Color rgba(std::uint32_t hex) {
    return {static_cast<std::uint8_t>(hex >> 24), static_cast<std::uint8_t>(hex >> 16), static_cast<std::uint8_t>(hex >> 8),
            static_cast<std::uint8_t>(hex)};
}

}  // namespace vs
