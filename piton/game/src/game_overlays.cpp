#include "game_overlays.hpp"

#include <algorithm>
#include <cmath>

#include "assets.hpp"
#include "easing.hpp"
#include "palette.hpp"
#include "playfield_view.hpp"
#include "ui_draw.hpp"

namespace piton_game {
namespace {

const vs::Font& font_of(const Assets& a, int size) {
    if (size <= 8) return a.font8;
    if (size <= 16) return a.font16;
    if (size <= 24) return a.font24;
    return a.font32;
}

constexpr vs::RectF kFieldRect{kFieldOrigin.x, kFieldOrigin.y, kFieldW, kFieldH};

// One HUD plate: small caption, big value.
void plate(vs::Renderer& r, const Assets& a, vs::RectF rc, const char* caption, const std::string& value,
           vs::Color value_color) {
    r.fill_rect({rc.x + 3, rc.y + 4, rc.w, rc.h}, vs::rgba(0x00000060));
    draw_nine_slice(r, a.ui, "panel", rc, 8, 1.0f);
    r.fill_rect(rc.inset(8), pal::kPanel);
    draw_text(r, a.font8, caption, {rc.x + rc.w * 0.5f, rc.y + 16}, pal::kTextDim, vs::Align::Center);
    draw_text(r, a.font24, value, {rc.x + rc.w * 0.5f, rc.y + 34}, value_color, vs::Align::Center);
}

}  // namespace

void draw_hud(vs::Renderer& r, const Assets& a, const HudInfo& info) {
    const float gap = 16;
    const float w = (kFieldW + 32 - 3 * gap) / 4;  // the plates span the field's frame
    const float x0 = kFieldOrigin.x - 16;
    const float y = 12;
    const float h = 74;
    plate(r, a, {x0, y, w, h}, "LEVEL", padded(info.level, 2), pal::kText);

    // Rabbits: count and a progress bar.
    const vs::RectF rc{x0 + (w + gap), y, w, h};
    plate(r, a, rc, "RABBITS", std::to_string(info.eaten) + "/" + std::to_string(info.target), pal::kText);
    a.ui.draw(r, "icon_rabbit", {rc.x + 14, rc.y + 32});
    const vs::RectF bar{rc.x + 14, rc.bottom() - 16, rc.w - 28, 4};
    r.fill_rect(bar, vs::rgba(0x00000080));
    const float k = info.target > 0 ? std::clamp(static_cast<float>(info.eaten) / static_cast<float>(info.target), 0.0f, 1.0f) : 0.0f;
    r.fill_rect({bar.x, bar.y, std::round(bar.w * k), bar.h}, pal::kGreen);

    plate(r, a, {x0 + 2 * (w + gap), y, w, h}, "SCORE", padded(info.score, 6), pal::kText);
    const bool record = info.mode == PlayMode::Play && info.score > 0 && info.score >= info.high;
    const vs::RectF hs{x0 + 3 * (w + gap), y, w, h};
    plate(r, a, hs, "HIGH SCORE", padded(info.high, 6), record ? pal::kGold : pal::kText);
    a.ui.draw(r, "icon_star", {hs.x + 12, hs.y + 10});

    // Level name centred between the HUD and the field.
    if (!info.level_name.empty())
        draw_text(r, a.font16, info.level_name, {640, y + h + 12}, pal::kText, vs::Align::Center, TextFx::Both);
}

void draw_banner(vs::Renderer& r, const Assets& a, const std::string& title, vs::Color title_color,
                 const std::vector<BannerLine>& lines, float appear) {
    const float k = ease::out_back(appear);
    r.fill_rect(kFieldRect, vs::Color{0, 0, 0, static_cast<std::uint8_t>(120 * ease::clamp01(appear))});
    float h = 96;
    for (const BannerLine& l : lines) h += static_cast<float>(l.size) + 18;
    const float w = 620;
    const vs::Vec2 c{kFieldRect.x + kFieldRect.w * 0.5f, kFieldRect.y + kFieldRect.h * 0.5f};
    const float sw = w * (0.6f + 0.4f * k);
    const float sh = h * (0.6f + 0.4f * k);
    if (appear <= 0) return;
    draw_panel(r, a, {std::round(c.x - sw * 0.5f), std::round(c.y - sh * 0.5f), std::round(sw), std::round(sh)});
    if (appear < 0.6f) return;
    float y = c.y - h * 0.5f + 34;
    draw_text(r, a.font32, title, {c.x, y}, title_color, vs::Align::Center, TextFx::Both);
    y += 32 + 26;
    for (const BannerLine& l : lines) {
        draw_text(r, font_of(a, l.size), l.text, {c.x, y}, l.color, vs::Align::Center);
        y += static_cast<float>(l.size) + 18;
    }
}

void draw_big_word(vs::Renderer& r, const Assets& a, const std::string& word, vs::Color color, float pop) {
    const vs::Vec2 c{kFieldRect.x + kFieldRect.w * 0.5f, kFieldRect.y + kFieldRect.h * 0.5f};
    const float k = ease::out_back(pop);
    const vs::Font& f = k > 0.85f ? a.font48 : k > 0.55f ? a.font32 : a.font24;
    r.fill_rect({kFieldRect.x, c.y - 52, kFieldRect.w, 104}, vs::rgba(0x00000070));
    draw_text(r, f, word, {c.x, c.y - f.size() * 0.5f}, color, vs::Align::Center, TextFx::Both);
}

void draw_mode_label(vs::Renderer& r, const Assets& a, PlayMode mode, double clock) {
    if (mode == PlayMode::Play) return;
    const bool on = std::fmod(clock, 1.0) < 0.7;
    const char* text = mode == PlayMode::Demo ? "DEMO" : "TEST PLAY";
    const vs::Vec2 c{kFieldRect.x + kFieldRect.w * 0.5f, kFieldRect.y + 14};
    const float w = a.font24.measure(text) + 40;
    r.fill_rect({c.x - w * 0.5f, c.y - 6, w, 38}, vs::rgba(0x000000A0));
    if (on) draw_text(r, a.font24, text, {c.x, c.y}, mode == PlayMode::Demo ? pal::kRed : pal::kOrange, vs::Align::Center,
                      TextFx::Both);
    if (mode == PlayMode::Demo)
        draw_text(r, a.font8, "PRESS ANY KEY", {c.x, kFieldRect.bottom() - 22}, pal::kText, vs::Align::Center, TextFx::Both);
}

}  // namespace piton_game
