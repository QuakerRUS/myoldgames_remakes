#pragma once
#include <string>

#include <vs/engine/sprite_sheet_data.hpp>

namespace vs {

// Plays an AnimationDef: tracks time and tells which frame to show.
// The AnimationDef must outlive the player (normally it lives in a SpriteSheet).
class AnimationPlayer {
public:
    AnimationPlayer() = default;
    explicit AnimationPlayer(const AnimationDef* def) { play(def, true); }

    // Starts `def`. If it is already the current animation and restart is false, it continues.
    void play(const AnimationDef* def, bool restart = false);
    void stop() { def_ = nullptr; index_ = 0; finished_ = false; elapsed_ms_ = 0; }
    // Advances by dt seconds (multiplied by speed()).
    void update(double dt);
    // Back to the first frame.
    void restart();

    void set_speed(double speed) { speed_ = speed; }
    double speed() const { return speed_; }

    const AnimationDef* animation() const { return def_; }
    bool playing() const { return def_ != nullptr && !finished_; }
    // Index of the current frame (0 when nothing plays).
    int frame_index() const { return index_; }
    // Name of the current frame (empty when nothing plays).
    const std::string& frame() const;
    // True when a non-looping animation has passed the end of its last frame
    // (the last frame keeps being shown).
    bool finished() const { return finished_; }
    // Milliseconds since the start of the current loop.
    double elapsed_ms() const { return elapsed_ms_; }

private:
    void sync_index();

    const AnimationDef* def_ = nullptr;
    double elapsed_ms_ = 0;
    double speed_ = 1.0;
    int index_ = 0;
    bool finished_ = false;
};

}  // namespace vs
