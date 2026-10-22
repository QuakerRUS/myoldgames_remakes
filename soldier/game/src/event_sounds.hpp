// Sound effects for the game's events, scheduled to match the animations (a box splashes
// when it reaches the water, a button clicks when it is stepped on, ...).
#pragma once

#include <string>
#include <vector>

#include <soldier/game.hpp>

namespace sg {

class Resources;
class LevelView;
struct ViewClock;

class EventSounds {
public:
    void on_event(const soldier::Event& e, const soldier::Game& game, const LevelView& view, const ViewClock& c);
    // Plays the scheduled sounds that are due at `now`.
    void update(Resources& res, double now);
    void clear() { queue_.clear(); }

private:
    struct Cue {
        double at;
        std::string name;
        float volume, pitch, pan;
    };
    std::vector<Cue> queue_;
    int steps_ = 0;
    void add(double at, std::string name, float volume = 1.0f, float pitch = 1.0f, float pan = 0.0f);
};

}  // namespace sg
