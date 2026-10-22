#pragma once
#include <cstdint>
#include <memory>
#include <optional>

#include <kbsim/layout.hpp>
#include <kbsim/session.hpp>
#include <vs/engine/scene.hpp>
#include <vs/engine/texture.hpp>
#include <vs/engine/ui_theme.hpp>

#include "fonts.hpp"
#include "keyboard_view.hpp"
#include "modal.hpp"
#include "options.hpp"
#include "settings.hpp"
#include "side_panel.hpp"
#include "sounds.hpp"
#include "text_view.hpp"

namespace ks {

// The whole program: one screen with the text, the stats, the buttons and the keyboard.
class TrainerScene : public vs::Scene {
public:
    explicit TrainerScene(Options options) : options_(std::move(options)) {}

    void on_enter() override;
    void on_exit() override;
    void on_event(const SDL_Event& e) override;
    // Window close (title bar X, Alt+F4): the same question as Esc; quits at once in smoke and
    // scripted (screenshot) runs.
    bool on_quit_request() override;
    void update(double dt) override;
    void render(vs::Renderer& r, double alpha) override;

private:
    std::int64_t now() const;  // the session clock (ms)
    void type_char(char32_t ch, std::int64_t at_ms, bool sounds);
    void restart();
    void toggle_pause();
    void toggle_sound();
    void open_modal(Modal::Spec spec);
    void ask_quit();  // «Выйти из тренажёра?» (Esc, window close)
    void resume_after_modal();
    void run_script_typing();
    void run_script_state();
    void draw_text_card(vs::Renderer& r) const;
    void draw_pause_badge(vs::Renderer& r) const;
    void draw_layout_warning(vs::Renderer& r) const;

    Options options_;
    Settings settings_;
    Fonts fonts_;
    Sounds sounds_;
    vs::Theme theme_;
    vs::Texture icon_;
    std::unique_ptr<kbsim::Layout> layout_;  // must outlive session_
    std::unique_ptr<kbsim::Session> session_;
    std::unique_ptr<TextView> text_view_;
    std::unique_ptr<KeyboardView> keyboard_;
    std::unique_ptr<SidePanel> panel_;
    std::unique_ptr<Modal> modal_;

    std::int64_t clock_offset_ = 0;  // shifts SDL_GetTicks() for scripted typing
    bool modal_paused_ = false;      // the session was paused by a dialog, not by the user
    bool quit_asked_ = false;        // the open dialog is the quit question
    double time_ = 0;                // seconds since the scene started (animations, script)
    int script_stage_ = 0;
    bool shot_due_ = false;
};

}  // namespace ks
