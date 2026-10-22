#pragma once
#include <functional>
#include <string>

#include <vs/engine/input.hpp>
#include <vs/engine/renderer.hpp>
#include <vs/engine/ui_theme.hpp>

namespace vs {

// Rounded push button with hover/pressed states (Keyboard Simulator's Restart, Pause, ...).
// Click = left button pressed and released inside the rect. An optional hotkey also clicks.
class Button {
public:
    Button(const Theme* theme, std::string label, RectF rect, std::function<void()> on_click = {})
        : label(std::move(label)), rect(rect), on_click(std::move(on_click)), theme_(theme) {}

    std::string label;
    RectF rect;
    std::function<void()> on_click;
    SDL_Keycode hotkey = SDLK_UNKNOWN;
    bool enabled = true;
    bool active = false;  // toggled look (e.g. Pause while paused)

    void update(const Input& input);
    void render(Renderer& renderer) const;
    bool hovered() const { return hovered_; }
    bool pressed() const { return pressed_; }

private:
    const Theme* theme_;
    bool hovered_ = false;
    bool pressed_ = false;
};

}  // namespace vs
