#pragma once
// The game screen's HUD and the messages drawn over the field (READY, PAUSED, GAME OVER,
// LEVEL n COMPLETE, the DEMO label).

#include <string>
#include <vector>

#include <vs/engine/renderer.hpp>

#include "scenes.hpp"

namespace piton_game {

struct Assets;

struct HudInfo {
    int level = 1;
    std::string level_name;
    int eaten = 0;
    int target = 50;
    int score = 0;
    int high = 0;
    PlayMode mode = PlayMode::Play;
};

// Four plates above the field: LEVEL, RABBITS (with a progress bar), SCORE, HIGH SCORE.
void draw_hud(vs::Renderer& r, const Assets& a, const HudInfo& info);

// A framed message box centred on the field; `appear` 0..1 animates it in.
struct BannerLine {
    std::string text;
    int size = 16;  // 8 / 16 / 24
    vs::Color color;
};
void draw_banner(vs::Renderer& r, const Assets& a, const std::string& title, vs::Color title_color,
                 const std::vector<BannerLine>& lines, float appear);

// Big centred word over the field (READY / GO!), scaled by `pop` (0..1 pop-in).
void draw_big_word(vs::Renderer& r, const Assets& a, const std::string& word, vs::Color color, float pop);

// Blinking "DEMO" label (or "TEST PLAY") at the top of the field.
void draw_mode_label(vs::Renderer& r, const Assets& a, PlayMode mode, double clock);

}  // namespace piton_game
