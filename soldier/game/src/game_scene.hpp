// Live play of one level: keyboard input exactly as in the original, progress and records.
#pragma once

#include <optional>

#include <soldier/recording.hpp>

#include "level_scene.hpp"

namespace sg {

struct GameOptions {
    int slot = -1;           // player slot (progress and records); -1 = nothing is saved
    bool test_level = false; // --test-level: quit the program when the level ends
};

class GameScene : public LevelScene {
public:
    GameScene(ResourcesPtr res, soldier::Level level, GameOptions opts);

    void on_enter() override;

protected:
    soldier::Game& game() override { return game_; }
    const soldier::Game& game() const override { return game_; }
    void advance_tick() override;
    double tick_length() const override;
    void handle_input() override;
    void after_ticks() override;
    void on_game_over() override;
    void on_continue() override;
    HudInfo hud_info() const override;
    std::string end_title() const override;
    std::string end_subtitle() const override;

private:
    soldier::Game game_;
    GameOptions opts_;
    std::optional<soldier::Dir> held_;  // the arrow pressed last (held to keep walking)
    int held_tick_ = -1;                // tick of the last move issued by holding
    std::optional<soldier::Recording> autoplay_;
    std::size_t autoplay_next_ = 0;
    bool new_record_ = false;

    void apply_autoplay();
    bool any_arrow_down(soldier::Dir& d) const;
};

}  // namespace sg
