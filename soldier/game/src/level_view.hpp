// Presentation of a running soldier::Game: turns the game's events into animations
// (smooth steps and pushes, buttons, doors, keys, sinking boxes, deaths, ...) and draws the
// map through a camera. The rules stay in soldier_core; this class only reads the game.
#pragma once

#include <optional>
#include <unordered_map>
#include <vector>

#include <vs/engine/camera.hpp>
#include <vs/engine/renderer.hpp>
#include <soldier/game.hpp>

#include "tile_art.hpp"

namespace sg {

class Resources;

inline constexpr float kCell = 32.0f;  // world pixels per cell

// Timing information of the scene that drives the game.
struct ViewClock {
    double now = 0;           // view time, seconds
    double tick_len = 0.5;    // seconds per game tick
    double tick_elapsed = 0;  // seconds since the current tick started
};

class LevelView {
public:
    explicit LevelView(const soldier::Game& game);

    // Feed every event of the game right after the apply()/tick() that produced it.
    void on_event(const soldier::Event& e, const ViewClock& c);

    // Draws the visible part of the map into cam.viewport.
    void render(vs::Renderer& r, const Resources& res, const vs::Camera2D& cam, const ViewClock& c) const;

    // World position (pixels) of the player's centre at time `now` (for the camera).
    vs::Vec2 player_center(double now) const;
    // True once the death (or victory) animation of the player has finished.
    bool ending_done(double now) const;
    // Time at which the last pushed box reaches its cell (sounds are scheduled with it).
    double box_arrival(soldier::Point at) const;
    // When the player's current step ends (now if standing).
    double step_end(double now) const;

private:
    struct Motion {
        soldier::Point from, to;
        double start = 0, dur = 0;
        bool push = false;
        int parity = 0;
    };
    struct BoxMotion {
        soldier::Point from, to;
        double start = 0, dur = 0;
        bool sinks = false;
    };
    enum class FxKind { KeyPickup, KeyDrop, ColorDoor, Reveal, Dust };
    struct Effect {
        FxKind kind;
        soldier::Point at;
        double start = 0;
        soldier::KeyColor color = soldier::KeyColor::Red;
    };
    struct ButtonAnim {
        double start = 0;
        bool press = true;
    };

    const soldier::Game* game_;
    std::optional<Motion> motion_;
    std::vector<BoxMotion> boxes_;
    std::vector<Effect> effects_;
    std::unordered_map<int, ButtonAnim> buttons_;
    int steps_ = 0;
    double turn_start_ = -10, bump_start_ = -10, idle_since_ = 0;
    soldier::Dir bump_dir_ = soldier::Dir::Down;
    bool doors_opening_ = false;
    double doors_start_ = -10;
    double last_press_time_ = -1;
    std::optional<soldier::Point> death_cell_;
    soldier::Tile death_tile_ = soldier::Tile::Floor;
    soldier::Ground death_ground_ = soldier::Ground::Floor;  // what the player died on
    double death_start_ = 0;
    std::optional<double> win_start_;

    int index(soldier::Point p) const { return p.y * game_->width() + p.x; }
    soldier::Tile at(int x, int y) const;
    TileArt art_at(int x, int y) const;  // tile_art(), keeping the ground under the dead player
    double step_duration(const ViewClock& c) const;
    const BoxMotion* box_motion_to(soldier::Point p, double now) const;
    bool hide_static_object(soldier::Point p, double now) const;

    void draw_ground(vs::Renderer& r, const Resources& res, const vs::Camera2D& cam, int x, int y, const TileArt& art,
                     const ViewClock& c) const;
    void draw_ground_overlays(vs::Renderer& r, const Resources& res, const vs::Camera2D& cam, int x, int y,
                              const TileArt& art) const;
    void draw_object(vs::Renderer& r, const Resources& res, const vs::Camera2D& cam, int x, int y, const TileArt& art,
                     const ViewClock& c) const;
    void draw_player(vs::Renderer& r, const Resources& res, const vs::Camera2D& cam, double now) const;
    void draw_effects(vs::Renderer& r, const Resources& res, const vs::Camera2D& cam, double now, bool ground) const;
};

}  // namespace sg
