#include "input_script.hpp"

#include <cstdlib>
#include <sstream>

#include <SDL3/SDL.h>

namespace sg {

void InputScript::parse(const std::string& spec) {
    std::stringstream all(spec);
    std::string part;
    while (std::getline(all, part, ';')) {
        std::stringstream ps(part);
        Item it;
        std::string what;
        if (!(ps >> it.at >> what)) continue;
        if (what == "text") {
            ps >> it.text;
        } else if (what == "close") {
            it.close = true;
        } else {
            it.key = what;
        }
        items_.push_back(it);
    }
}

void InputScript::pump(double now) {
    while (!items_.empty() && items_.front().at <= now) {
        const Item it = items_.front();
        items_.pop_front();
        if (it.close) {
            SDL_Event e{};
            e.type = SDL_EVENT_QUIT;  // as if the window were closed
            SDL_PushEvent(&e);
        } else if (!it.key.empty()) {
            const SDL_Keycode k = SDL_GetKeyFromName(it.key.c_str());
            if (k == SDLK_UNKNOWN) {
                SDL_Log("script: unknown key %s", it.key.c_str());
                continue;
            }
            for (const Uint32 type : {static_cast<Uint32>(SDL_EVENT_KEY_DOWN), static_cast<Uint32>(SDL_EVENT_KEY_UP)}) {
                SDL_Event e{};
                e.type = type;
                e.key.key = k;
                e.key.down = type == SDL_EVENT_KEY_DOWN;
                e.key.windowID = 0;
                SDL_PushEvent(&e);
            }
        } else {
            texts_.push_back(it.text);
            SDL_Event e{};
            e.type = SDL_EVENT_TEXT_INPUT;
            e.text.text = texts_.back().c_str();
            SDL_PushEvent(&e);
        }
    }
}

}  // namespace sg
