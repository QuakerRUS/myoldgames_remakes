#include "draw_util.hpp"

#include <algorithm>
#include <cmath>

#include <kbsim/utf8.hpp>

namespace ks {

namespace {
std::uint8_t lerp8(std::uint8_t a, std::uint8_t b, float t) {
    return static_cast<std::uint8_t>(std::lround(static_cast<float>(a) + (static_cast<float>(b) - a) * t));
}
}  // namespace

vs::Color mix(vs::Color a, vs::Color b, float t) {
    t = clamp01(t);
    return {lerp8(a.r, b.r, t), lerp8(a.g, b.g, t), lerp8(a.b, b.b, t), lerp8(a.a, b.a, t)};
}

vs::Color fade(vs::Color c, float alpha_mul) {
    return c.with_alpha(static_cast<std::uint8_t>(std::lround(static_cast<float>(c.a) * clamp01(alpha_mul))));
}

float clamp01(float v) { return std::clamp(v, 0.0f, 1.0f); }

float ease_out(float t) {
    t = 1.0f - clamp01(t);
    return 1.0f - t * t * t;
}

float approach(float current, float target, float rate, double dt) {
    const float k = 1.0f - std::exp(-rate * static_cast<float>(dt));
    return current + (target - current) * k;
}

vs::RectF expand(vs::RectF rect, float d) { return {rect.x - d, rect.y - d, rect.w + 2 * d, rect.h + 2 * d}; }
vs::RectF offset(vs::RectF rect, float dx, float dy) { return {rect.x + dx, rect.y + dy, rect.w, rect.h}; }

void draw_shadow(vs::Renderer& r, vs::RectF rect, float radius, float strength) {
    constexpr int kLayers = 6;
    for (int i = kLayers; i >= 1; --i) {
        const float d = static_cast<float>(i) * 1.6f;
        const float a = 7.0f * strength * (1.0f - static_cast<float>(i - 1) / kLayers);
        r.fill_rounded_rect(offset(expand(rect, d), 0, 2.0f + d * 0.5f), radius + d,
                            {20, 32, 64, static_cast<std::uint8_t>(std::clamp(a, 0.0f, 255.0f))});
    }
}

void draw_card(vs::Renderer& r, vs::RectF rect, float radius, vs::Color fill, float shadow) {
    if (shadow > 0) draw_shadow(r, rect, radius, shadow);
    r.fill_rounded_rect(rect, radius, fill);
    r.draw_rounded_rect(rect, radius, {24, 40, 80, 18}, 1.0f);
}

void draw_glow(vs::Renderer& r, vs::RectF rect, float radius, vs::Color color, float spread, int layers) {
    for (int i = layers; i >= 1; --i) {
        const float t = static_cast<float>(i) / static_cast<float>(layers);
        const float d = spread * t;
        r.draw_rounded_rect(expand(rect, d), radius + d, fade(color, (1.0f - t) * 0.9f + 0.1f), spread / layers + 0.5f);
    }
}

std::string to_utf8(std::u32string_view text) {
    try {
        return kbsim::u32_to_utf8(text);
    } catch (...) {
        return "?";
    }
}

std::string to_utf8(char32_t ch) { return to_utf8(std::u32string_view(&ch, 1)); }

}  // namespace ks
