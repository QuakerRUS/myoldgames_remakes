// Shared part of the game and replay scenes: the tick clock, the map view and camera, the
// side panel, sounds and music, and the end-of-level message.
#pragma once

#include <memory>
#include <optional>
#include <string>

#include <vs/engine/camera.hpp>
#include <soldier/game.hpp>

#include "base_scene.hpp"
#include "event_sounds.hpp"
#include "hud.hpp"
#include "level_view.hpp"

namespace sg {

class LevelScene : public BaseScene {
public:
    LevelScene(ResourcesPtr res, soldier::Level level);

protected:
    // ---- hooks for the game / replay scenes ----
    virtual soldier::Game& game() = 0;
    virtual const soldier::Game& game() const = 0;
    virtual void advance_tick() = 0;           // Game::tick() or Replayer::step()
    virtual double tick_length() const = 0;    // seconds per tick
    virtual void handle_input() = 0;           // before the ticks of this step
    virtual void after_ticks() {}              // after the ticks (held keys)
    virtual void on_game_over() {}             // once, when the game stops playing
    virtual void on_continue() = 0;            // a key after the end message
    virtual HudInfo hud_info() const = 0;
    // Message shown when the level is over.
    virtual std::string end_title() const;
    virtual std::string end_subtitle() const;
    virtual double ending_pause() const { return 0.6; }

    // Applies the ZOOM setting (2x / 1x) and re-centres the camera on the player.
    void apply_zoom();
    // Toggles the ZOOM setting (Z key) and saves it.
    void toggle_zoom();
    // Game music (the fast version during the last 15 timer units).
    void start_music();
    // Call at the end of the derived constructor (the game exists then).
    void init_view();
    // Moves the game's pending events into the view and the sound queue.
    void flush_events();
    // Apply a command now (live input) and animate its events.
    void apply(soldier::Command c);

    void tick(double dt) override;
    void draw(vs::Renderer& r) override;
    bool screenshot_due() const override;

    ViewClock clock() const;

    soldier::Level level_;
    std::unique_ptr<LevelView> view_;
    vs::Camera2D cam_;
    EventSounds sounds_;
    double acc_ = 0;  // seconds since the current tick began
    bool paused_ = false;
    bool fast_music_ = false;
    bool over_ = false;          // on_game_over() was called
    bool prompt_ = false;        // "PRESS ANY KEY" is shown
    double over_time_ = 0;
    double shot_reached_ = -1;   // --shot-tick bookkeeping
    int max_ticks_per_step_ = 16;

private:
    void draw_end_message(vs::Renderer& r);
};

}  // namespace sg
