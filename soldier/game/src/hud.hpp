// The side panel of the game and replay screens: level, timer, inventory, key help.
#pragma once

#include <string>

#include <vs/engine/renderer.hpp>
#include <soldier/game.hpp>

namespace sg {

class Resources;

inline constexpr float kPanelWidth = 288.0f;
inline constexpr float kPanelX = 1280.0f - kPanelWidth;

struct HudInfo {
    int level = 0;
    bool replay = false;
    std::string replay_speed;  // "x1", "x2", "x4"
    std::string player;        // whose run is shown (replay)
    bool paused = false;
};

void draw_hud(vs::Renderer& r, const Resources& res, const soldier::Game& game, const HudInfo& info, double now);

// Colour of the timer value as in the original: green > 59, yellow 15..59, red < 15.
vs::Color timer_color(int value);

}  // namespace sg
