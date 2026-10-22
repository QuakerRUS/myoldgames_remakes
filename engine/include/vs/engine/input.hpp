#pragma once
#include <string>
#include <unordered_set>

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_keycode.h>

#include <vs/engine/math.hpp>

struct SDL_Window;

namespace vs {

// Keyboard and mouse state, fed by App from SDL events (mouse coordinates already converted
// to logical 1280x720 coordinates).
//
// "Edge" queries (pressed, released, repeated, text, wheel, mouse_pressed/released) collect the
// events that arrived since the previous logic step and are true during exactly ONE update()
// call (App calls end_step() after each update), so no key press is lost or seen twice when
// a frame runs zero or several fixed steps. down() / mouse_down() are the live state.
// Keys are SDL_Keycode values (layout-aware: SDLK_A, SDLK_RETURN, SDLK_LEFT, ...).
class Input {
public:
    // --- keyboard ---
    bool down(SDL_Keycode key) const { return down_.count(key) != 0; }
    bool pressed(SDL_Keycode key) const { return pressed_.count(key) != 0; }    // went down (no auto-repeat)
    bool released(SDL_Keycode key) const { return released_.count(key) != 0; }
    // Pressed or auto-repeated (OS key repeat) — for menu navigation and Backspace.
    bool repeated(SDL_Keycode key) const { return repeated_.count(key) != 0; }
    bool any_pressed() const { return !pressed_.empty() || !mouse_pressed_.empty(); }  // keys or mouse buttons
    SDL_Keymod mods() const { return mods_; }
    bool ctrl() const { return (mods_ & SDL_KMOD_CTRL) != 0; }
    bool shift() const { return (mods_ & SDL_KMOD_SHIFT) != 0; }
    bool alt() const { return (mods_ & SDL_KMOD_ALT) != 0; }

    // --- text input (UTF-8, from SDL_EVENT_TEXT_INPUT) ---
    // Text typed since the last step; only filled while text input is active.
    const std::string& text() const { return text_; }
    void start_text_input();
    void stop_text_input();
    bool text_input_active() const { return text_active_; }

    // --- mouse (logical coordinates) ---
    Vec2 mouse() const { return mouse_; }
    bool mouse_moved() const { return mouse_moved_; }
    // button: SDL_BUTTON_LEFT (1), SDL_BUTTON_MIDDLE (2), SDL_BUTTON_RIGHT (3)
    bool mouse_down(int button) const { return mouse_down_.count(button) != 0; }
    bool mouse_pressed(int button) const { return mouse_pressed_.count(button) != 0; }
    bool mouse_released(int button) const { return mouse_released_.count(button) != 0; }
    float wheel() const { return wheel_; }  // vertical wheel steps since the last step (+ = away from user)

    // --- used by App ---
    void attach(SDL_Window* window) { window_ = window; }
    void handle_event(const SDL_Event& e);
    void end_step();
    // Forgets held keys (e.g. on focus loss) so that no key stays stuck.
    void reset();

private:
    SDL_Window* window_ = nullptr;
    std::unordered_set<SDL_Keycode> down_, pressed_, released_, repeated_;
    std::unordered_set<int> mouse_down_, mouse_pressed_, mouse_released_;
    SDL_Keymod mods_ = SDL_KMOD_NONE;
    std::string text_;
    bool text_active_ = false;
    Vec2 mouse_{};
    bool mouse_moved_ = false;
    float wheel_ = 0.0f;
};

}  // namespace vs
