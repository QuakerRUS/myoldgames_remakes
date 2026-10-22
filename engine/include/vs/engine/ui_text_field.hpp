#pragma once
#include <cstddef>
#include <functional>
#include <string>

#include <vs/engine/input.hpp>
#include <vs/engine/renderer.hpp>
#include <vs/engine/ui_theme.hpp>

namespace vs {

// Single-line text entry. Takes characters from Input::text() (so the owner must call
// input.start_text_input() while editing and stop_text_input() afterwards), Backspace (with
// repeat) deletes, Enter calls on_submit, Esc calls on_cancel. The caret is always at the end.
class TextField {
public:
    explicit TextField(const Theme* theme) : theme_(theme) {}

    std::string text;             // UTF-8
    std::size_t max_length = 32;  // in code points
    bool uppercase = false;       // convert letters to upper case before filtering
    // Allowed characters; empty = any printable character (>= U+0020).
    std::function<bool(char32_t)> filter;
    RectF rect{0, 0, 300, 48};
    Align align = Align::Left;    // text alignment inside the box
    bool draw_box = true;         // background and border
    std::string placeholder;      // shown dimmed while text is empty

    std::function<void(const std::string&)> on_submit;
    std::function<void()> on_cancel;

    // Call from Scene::update with the fixed dt (dt drives the caret blink).
    void update(const Input& input, double dt);
    void render(Renderer& renderer) const;

    // Applies typed UTF-8 text through uppercase/filter/max_length. Returns true if anything
    // was added. (Used by update; public for tests and for feeding text manually.)
    bool insert(std::string_view utf8);

    // Filter helpers: A-Z and 0-9 (Soldier player names), digits only.
    static bool latin_upper_or_digit(char32_t c) { return (c >= U'A' && c <= U'Z') || (c >= U'0' && c <= U'9'); }
    static bool digit(char32_t c) { return c >= U'0' && c <= U'9'; }

private:
    const Theme* theme_;
    double blink_ = 0.0;
};

}  // namespace vs
