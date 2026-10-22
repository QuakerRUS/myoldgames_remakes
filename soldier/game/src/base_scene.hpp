// Common base of the game's scenes: shared resources, a scene clock (with the debug time
// scale), integer scaling of the canvas when the window is large enough, and the
// --screenshot hook used by automated visual checks.
#pragma once

#include <string>
#include <vector>

#include <SDL3/SDL_keycode.h>
#include <vs/engine/app.hpp>
#include <vs/engine/scene.hpp>

#include "resources.hpp"
#include "ui_kit.hpp"

namespace sg {

class BaseScene : public vs::Scene {
public:
    explicit BaseScene(ResourcesPtr res) : res_(std::move(res)) {}

    void on_event(const SDL_Event& e) override;
    void update(double dt) final;
    void render(vs::Renderer& r, double alpha) final;
    // Closing the window asks "EXIT SOLDIER?" (as Esc in the main menu does); --smoke,
    // --verify-replays and --test-level (the editor's F5) quit at once.
    bool on_quit_request() final;

protected:
    // Scene logic with the scaled dt.
    virtual void tick(double dt) = 0;
    virtual void draw(vs::Renderer& r) = 0;
    // When this returns true (and --screenshot is set), the frame is saved and the program quits.
    virtual bool screenshot_due() const;
    // Opens the "EXIT SOLDIER?" question (the scene is frozen while it is open); YES saves the
    // settings and quits.
    void ask_quit();
    bool quit_question_open() const { return quit_box_.is_open(); }

    Resources& res() const { return *res_; }
    ResourcesPtr shared_res() const { return res_; }
    vs::Input& input() const { return app().input(); }
    // Scene time in seconds (scaled), interpolated between logic steps while rendering.
    double time() const { return time_; }
    // Keys pressed since the last tick, in order (no auto-repeat).
    const std::vector<SDL_Keycode>& keys() const { return keys_; }

private:
    ResourcesPtr res_;
    double time_ = 0;
    double logic_time_ = 0;
    double last_dt_ = 0;
    std::vector<SDL_Keycode> pending_keys_, keys_;
    bool shot_done_ = false;
    ConfirmBox quit_box_;
};

// Chooses integer scaling when the window can show the canvas at least 1:1.
void update_canvas_scaling(vs::App& app);

}  // namespace sg
