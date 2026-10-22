#pragma once
// Everything drawn inside the 912x552 field: ground, rabbit, snake, score pop-ups. Reacts to
// the game's tick events (animations) but owns no rules.

#include <string>
#include <vector>

#include <vs/engine/renderer.hpp>

#include <piton/game.hpp>

#include "field_renderer.hpp"
#include "head_animator.hpp"
#include "rabbit_view.hpp"

namespace piton_game {

struct Assets;

// Field position on the 1280x720 canvas.
inline constexpr vs::Vec2 kFieldOrigin{(1280 - 912) / 2.0f, 146};

class PlayfieldView {
public:
    void init(const Assets& a, const piton::Game& game, unsigned seed);
    // Animations for the events of the last tick; `points` = score for an eaten rabbit.
    void on_events(const piton::Game& game, const std::vector<piton::Event>& events, int points, double tick_seconds);
    void update(double dt, const piton::Game& game);
    void shake(double seconds) { shake_ = seconds; }
    // progress = 0..1 between the last tick and the next.
    void draw(vs::Renderer& r, const Assets& a, const piton::Game& game, float progress) const;

private:
    struct Popup {
        vs::Vec2 pos;
        std::string text;
        double age = 0;
    };
    FieldRenderer field_;
    RabbitView rabbit_;
    HeadAnimator head_;
    std::vector<Popup> popups_;
    double shake_ = 0;
    double clock_ = 0;
};

}  // namespace piton_game
