#include <vs/engine/input.hpp>

#include <SDL3/SDL.h>

namespace vs {

void Input::start_text_input() {
    if (window_ && !text_active_) SDL_StartTextInput(window_);
    text_active_ = true;
}

void Input::stop_text_input() {
    if (window_ && text_active_) SDL_StopTextInput(window_);
    text_active_ = false;
    text_.clear();
}

void Input::handle_event(const SDL_Event& e) {
    switch (e.type) {
    case SDL_EVENT_KEY_DOWN:
        mods_ = e.key.mod;
        if (!e.key.repeat) {
            down_.insert(e.key.key);
            pressed_.insert(e.key.key);
        }
        repeated_.insert(e.key.key);
        break;
    case SDL_EVENT_KEY_UP:
        mods_ = e.key.mod;
        down_.erase(e.key.key);
        released_.insert(e.key.key);
        break;
    case SDL_EVENT_TEXT_INPUT:
        if (text_active_ && e.text.text) text_ += e.text.text;
        break;
    case SDL_EVENT_MOUSE_MOTION:
        mouse_ = {e.motion.x, e.motion.y};
        mouse_moved_ = true;
        break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
        mouse_ = {e.button.x, e.button.y};
        mouse_down_.insert(e.button.button);
        mouse_pressed_.insert(e.button.button);
        break;
    case SDL_EVENT_MOUSE_BUTTON_UP:
        mouse_ = {e.button.x, e.button.y};
        mouse_down_.erase(e.button.button);
        mouse_released_.insert(e.button.button);
        break;
    case SDL_EVENT_MOUSE_WHEEL:
        wheel_ += e.wheel.direction == SDL_MOUSEWHEEL_FLIPPED ? -e.wheel.y : e.wheel.y;
        break;
    case SDL_EVENT_WINDOW_FOCUS_LOST:
        reset();
        break;
    default:
        break;
    }
}

void Input::end_step() {
    pressed_.clear();
    released_.clear();
    repeated_.clear();
    mouse_pressed_.clear();
    mouse_released_.clear();
    text_.clear();
    mouse_moved_ = false;
    wheel_ = 0.0f;
}

void Input::reset() {
    for (SDL_Keycode k : down_) released_.insert(k);
    down_.clear();
    mouse_down_.clear();
    mods_ = SDL_KMOD_NONE;
}

}  // namespace vs
