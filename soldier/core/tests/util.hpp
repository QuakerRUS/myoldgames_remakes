// Test helpers: build levels from the one-character tile notation of the level JSON.
#pragma once

#include <doctest/doctest.h>

#include <initializer_list>
#include <string>
#include <vector>

#include "soldier/game.hpp"

namespace soldier::test {

inline Level make_level(std::initializer_list<std::string> rows, int time_limit = 0) {
    Level lv;
    lv.number = 99;
    lv.height = static_cast<int>(rows.size());
    lv.width = static_cast<int>(rows.begin()->size());
    lv.time_limit = time_limit;
    for (const std::string& r : rows) {
        REQUIRE(static_cast<int>(r.size()) == lv.width);
        for (char c : r) {
            const auto t = tile_from_char(c);
            REQUIRE_MESSAGE(t.has_value(), "bad tile char ", c);
            lv.cells.push_back(*t);
        }
    }
    return lv;
}

inline std::string row(const Game& g, int y) {
    std::string s;
    for (int x = 0; x < g.width(); ++x) s += tile_char(g.cell({x, y}));
    return s;
}

// Moves after waiting out the cooldown, so that every call is executed immediately.
inline void go(Game& g, Dir d) {
    while (!g.can_move_now()) g.tick();
    g.apply(Command::move(d));
}

inline bool has_event(const std::vector<Event>& ev, EventType t) {
    for (const Event& e : ev)
        if (e.type == t) return true;
    return false;
}

inline std::string data_path(const std::string& rel) { return std::string(SOLDIER_DATA_DIR) + "/" + rel; }

inline std::string two_digits(int n) { return (n < 10 ? "0" : "") + std::to_string(n); }

}  // namespace soldier::test
