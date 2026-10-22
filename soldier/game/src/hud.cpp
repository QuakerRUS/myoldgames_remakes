#include "hud.hpp"

#include <cmath>

#include "resources.hpp"
#include "tile_art.hpp"
#include "ui_kit.hpp"

namespace sg {

namespace {

constexpr float kCenter = kPanelX + 10 + (kPanelWidth - 10) / 2;  // the hazard stripe is 10 px
constexpr float kInner = 236;                                     // width of the boxes
constexpr float kSlot = 40, kGap = 6;

void value_box(vs::Renderer& r, const Resources& res, float y, std::string_view icon, std::string_view label,
               const std::string& value, vs::Color value_color) {
    const vs::RectF box{kCenter - kInner / 2, y, kInner, 52};
    draw_inset(r, res, box);
    vs::DrawParams p;
    p.scale = {2, 2};
    res.ui.draw(r, icon, {box.x + 12, box.y + 10}, p);
    draw_text(r, res.font16, label, {box.x + 50, box.y + 18}, colors::dim);
    draw_text(r, res.font24, value, {box.right() - 12, box.y + 14}, value_color, vs::Align::Right);
}

}  // namespace

vs::Color timer_color(int value) {
    if (value > 59) return colors::green;
    if (value >= 15) return colors::yellow;
    return colors::red;
}

void draw_hud(vs::Renderer& r, const Resources& res, const soldier::Game& game, const HudInfo& info, double now) {
    r.draw(res.panel_bg, std::nullopt, {kPanelX, 0});

    // Title.
    {
        const float w = 236;
        const float s = w / static_cast<float>(res.logo.width());
        vs::DrawParams p;
        p.scale = {s, s};
        r.draw(res.logo, std::nullopt, {kCenter - w / 2, 22}, p);
    }

    float y = 92;
    value_box(r, res, y, "icon_level", "LEVEL", std::to_string(info.level), colors::text);
    y += 64;
    if (game.timer() >= 0) {
        const int t = game.timer();
        vs::Color c = timer_color(t);
        if (t < 15 && game.status() == soldier::Status::Playing && std::fmod(now, 1.0) < 0.25) c = colors::text;
        value_box(r, res, y, "icon_timer", "TIME", std::to_string(t), c);
        y += 64;
    }

    // Inventory: 5x5 grid of slots with the cursor.
    y += 6;
    draw_text(r, res.font16, "INVENTORY", {kCenter, y}, colors::cyan, vs::Align::Center);
    y += 26;
    const float grid = kSlot * 5 + kGap * 4;
    const float gx = kCenter - grid / 2;
    const auto& items = game.inventory();
    for (int i = 0; i < soldier::kInventorySize; ++i) {
        const float sx = gx + static_cast<float>(i % 5) * (kSlot + kGap);
        const float sy = y + static_cast<float>(i / 5) * (kSlot + kGap);
        res.ui.draw(r, "inv_slot", {sx, sy});
        if (i < static_cast<int>(items.size()))
            res.tiles.draw(r, std::string("key_") + color_name(items[static_cast<std::size_t>(i)]), {sx + 4, sy + 4});
    }
    if (!items.empty()) {
        const int i = game.inventory_cursor();
        const float sx = gx + static_cast<float>(i % 5) * (kSlot + kGap);
        const float sy = y + static_cast<float>(i / 5) * (kSlot + kGap);
        res.ui.draw(r, frame_at(res.ui.animation("cursor"), now * 1000.0), {sx - 2, sy - 2});
    }
    y += grid + 18;

    // Bottom: replay status or the key help.
    if (info.replay) {
        const vs::RectF box{kCenter - kInner / 2, y, kInner, 140};
        draw_inset(r, res, box);
        const uint8_t a = static_cast<uint8_t>(170 + 85 * std::sin(now * 4.0));
        draw_text(r, res.font24, "REPLAY", {kCenter, box.y + 14}, colors::red.with_alpha(a), vs::Align::Center);
        draw_text(r, res.body_small, info.player + "   SPEED " + info.replay_speed, {kCenter, box.y + 46}, colors::text,
                  vs::Align::Center);
        draw_text(r, res.body_small, "1 / 2 / 4 / TAB: SPEED", {kCenter, box.y + 70}, colors::dim, vs::Align::Center);
        draw_text(r, res.body_small, "Z: ZOOM", {kCenter, box.y + 90}, colors::dim, vs::Align::Center);
        draw_text(r, res.body_small, "OTHER KEYS: EXIT", {kCenter, box.y + 110}, colors::dim, vs::Align::Center);
    } else {
        static const char* help[][2] = {
            {"ARROWS", "MOVE / PUSH"}, {", .", "TURN"}, {"TAB", "NEXT ITEM"},
            {"SPACE", "USE ITEM"},     {"Z", "ZOOM"},   {"P", "PAUSE"},
            {"ESC", "GIVE UP"},
        };
        draw_inset(r, res, {kCenter - kInner / 2, y, kInner, 7 * 21 + 16});
        float hy = y + 8;
        for (const auto& h : help) {
            draw_text(r, res.body_small, h[0], {kCenter - 14, hy}, colors::accent, vs::Align::Right);
            draw_text(r, res.body_small, h[1], {kCenter + 2, hy}, colors::text);
            hy += 21;
        }
    }
}

}  // namespace sg
