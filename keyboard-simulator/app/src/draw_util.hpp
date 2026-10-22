#pragma once
#include <string>
#include <string_view>

#include <vs/engine/math.hpp>
#include <vs/engine/renderer.hpp>

namespace ks {

vs::Color mix(vs::Color a, vs::Color b, float t);       // t = 0 -> a, 1 -> b (alpha too)
vs::Color fade(vs::Color c, float alpha_mul);             // multiplies the alpha
float clamp01(float v);
float ease_out(float t);                                  // cubic ease-out on [0, 1]
float approach(float current, float target, float rate, double dt);  // exponential smoothing

// Soft drop shadow under a rounded rectangle (several translucent layers).
void draw_shadow(vs::Renderer& r, vs::RectF rect, float radius, float strength = 1.0f);
// White card: shadow + fill + hairline border.
void draw_card(vs::Renderer& r, vs::RectF rect, float radius, vs::Color fill, float shadow = 1.0f);
// Glow around a rounded rectangle: `layers` outlines fading out over `spread` pixels.
void draw_glow(vs::Renderer& r, vs::RectF rect, float radius, vs::Color color, float spread, int layers = 5);
vs::RectF expand(vs::RectF rect, float d);
vs::RectF offset(vs::RectF rect, float dx, float dy);

std::string to_utf8(std::u32string_view text);
std::string to_utf8(char32_t ch);

}  // namespace ks
