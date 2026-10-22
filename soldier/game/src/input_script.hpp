// --script "<t> <key>; <t> text <TEXT>; ...": a debug feature for automated checks of the
// menus and the game flow. At real time t (seconds since start) the key is pressed and
// released (or the text is typed) by pushing ordinary SDL events. Key names are SDL key
// names ("Return", "Tab", "Escape", "Up", "Space", "1", ...); "<t> close" requests closing
// the window.
#pragma once

#include <deque>
#include <string>

namespace sg {

class InputScript {
public:
    void parse(const std::string& spec);
    // Pushes the events that are due at `now`.
    void pump(double now);
    bool empty() const { return items_.empty(); }

private:
    struct Item {
        double at = 0;
        std::string key;   // SDL key name, or empty for text
        std::string text;  // text to type
        bool close = false;  // "<t> close": a window close request (SDL_EVENT_QUIT)
    };
    std::deque<Item> items_;
    std::deque<std::string> texts_;  // keeps pushed text alive (SDL does not copy it)
};

}  // namespace sg
