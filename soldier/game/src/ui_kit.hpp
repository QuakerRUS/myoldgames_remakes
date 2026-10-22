// Small drawing helpers and modal boxes in the game's riveted-steel style (ui.png).
#pragma once

#include <functional>
#include <string>

#include <vs/engine/input.hpp>
#include <vs/engine/renderer.hpp>
#include <vs/engine/sprite_sheet.hpp>

#include "resources.hpp"

namespace sg {

// Draws a 9-slice frame: the corners keep their size, edges and centre stretch.
void draw_nine(vs::Renderer& r, const vs::SpriteSheet& sheet, std::string_view frame, int border, vs::RectF dst,
               vs::Color tint = {255, 255, 255, 255});

// Riveted panel (frame9) / recessed display (inset9) / button (button9, button9_hot).
void draw_panel(vs::Renderer& r, const Resources& res, vs::RectF dst, uint8_t alpha = 255);
void draw_inset(vs::Renderer& r, const Resources& res, vs::RectF dst);
void draw_button(vs::Renderer& r, const Resources& res, vs::RectF dst, bool hot);

// Text with the standard drop shadow.
vs::Vec2 draw_text(vs::Renderer& r, const vs::Font& f, std::string_view text, vs::Vec2 pos, vs::Color c,
                   vs::Align align = vs::Align::Left);

// Animated arrow marker (menu_arrow) whose tip points right at `pos` (vertically centred).
void draw_marker(vs::Renderer& r, const Resources& res, vs::Vec2 pos, double time);

// Full-screen dimming.
void dim_screen(vs::Renderer& r, uint8_t alpha);

// A modal message ("ACCESS DENIED" + "PRESS ANY KEY TO CONTINUE"); any key closes it.
class NoticeBox {
public:
    void open(std::string title, std::string line = {}, vs::Color title_color = colors::red,
              std::function<void()> on_close = {});
    bool is_open() const { return open_; }
    // Returns true while open (the caller skips its own input then).
    bool update(const vs::Input& in, double dt);
    void render(vs::Renderer& r, const Resources& res) const;

private:
    bool open_ = false;
    std::string title_, line_;
    vs::Color color_{};
    std::function<void()> on_close_;
    double t_ = 0;
};

// A modal YES/NO question; Y/N hotkeys, Left/Right/Tab, Enter/Space, Esc = NO.
class ConfirmBox {
public:
    void open(std::string question, std::function<void(bool)> on_answer, bool default_yes = false);
    bool is_open() const { return open_; }
    bool update(const vs::Input& in, Resources& res, double dt);
    void render(vs::Renderer& r, const Resources& res) const;

private:
    bool open_ = false;
    bool yes_ = false;
    std::string question_;
    std::function<void(bool)> on_answer_;
    double t_ = 0;
    vs::RectF button_rect(bool yes) const;
    void finish(bool yes, Resources& res);
};

}  // namespace sg
