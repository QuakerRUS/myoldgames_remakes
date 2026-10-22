// Playback of a recording with the game's renderer (soldier::Replayer). Speed x1/x2/x4 with
// the keys 1/2/4 or Tab; any other key leaves.
#pragma once

#include <functional>
#include <string>

#include <soldier/recording.hpp>

#include "level_scene.hpp"

namespace sg {

struct ReplayResult {
    int level = 0;
    soldier::Status status = soldier::Status::Playing;
    int tick = 0;
    int expected_tick = -1;  // from the recording's result, if any
};

class ReplayScene : public LevelScene {
public:
    ReplayScene(ResourcesPtr res, soldier::Level level, soldier::Recording rec);

    // Verification mode: runs as fast as possible, silently, and reports the outcome
    // instead of waiting for a key (the callback decides what comes next).
    void set_verify(std::function<void(const ReplayResult&)> done);

    void on_enter() override;

protected:
    soldier::Game& game() override { return replayer_.game(); }
    const soldier::Game& game() const override { return replayer_.game(); }
    void advance_tick() override;
    double tick_length() const override;
    void handle_input() override;
    void on_game_over() override;
    void on_continue() override;
    HudInfo hud_info() const override;
    std::string end_title() const override;
    double ending_pause() const override { return verify_ ? 0.0 : 0.6; }

private:
    soldier::Replayer replayer_;
    std::string player_;
    int expected_tick_ = -1;
    int last_command_tick_ = 0;
    int speed_ = 1;
    std::function<void(const ReplayResult&)> verify_;
    bool reported_ = false;
};

}  // namespace sg
