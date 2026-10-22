#pragma once
// Small drawing helpers shared by the scenes: wooden 9-slice panels, text with shadow /
// outline, the dimmed jungle background.

#include <string_view>

#include <vs/engine/font.hpp>
#include <vs/engine/renderer.hpp>
#include <vs/engine/sprite_sheet.hpp>

namespace piton_game {

struct Assets;

// Draws frame `name` of `sheet` as a 9-slice into `dst`; `border` is in source pixels, the
// corners are drawn at `scale`.
void draw_nine_slice(vs::Renderer& r, const vs::SpriteSheet& sheet, std::string_view name, vs::RectF dst, int border,
                     float scale = 1.0f);

// Wooden frame (ui.json "panel") drawn around `inner`, with a soft drop shadow.
void draw_wood_frame(vs::Renderer& r, const Assets& a, vs::RectF inner, float thickness = 16.0f);

// Dark translucent panel with a wooden frame.
void draw_panel(vs::Renderer& r, const Assets& a, vs::RectF rect);

enum class TextFx { Shadow, Outline, Both };

// Text with the game's usual shadow/outline. Returns the drawn size.
vs::Vec2 draw_text(vs::Renderer& r, const vs::Font& f, std::string_view text, vs::Vec2 pos, vs::Color color,
                   vs::Align align = vs::Align::Left, TextFx fx = TextFx::Shadow);

// Jungle backdrop (menu_bg.png) darkened by `dim` (0 = as is, 1 = black).
void draw_backdrop(vs::Renderer& r, const Assets& a, float dim);

// "000123"-style zero padded number.
std::string padded(int value, int digits);

}  // namespace piton_game
