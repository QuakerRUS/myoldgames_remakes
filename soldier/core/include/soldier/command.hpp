// Player commands (what the keyboard produces).
#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

#include "soldier/tile.hpp"

namespace soldier {

enum class CommandType : std::uint8_t {
    Move,      // arrow key: step / push / bump in `dir`
    TurnCW,    // "." or ">"
    TurnCCW,   // "," or "<"
    NextItem,  // Tab
    UseItem,   // Space or Enter
    GiveUp,    // Esc
};

struct Command {
    CommandType type = CommandType::Move;
    Dir dir = Dir::Down;  // meaningful only for Move

    static constexpr Command move(Dir d) { return {CommandType::Move, d}; }
    static constexpr Command turn_cw() { return {CommandType::TurnCW}; }
    static constexpr Command turn_ccw() { return {CommandType::TurnCCW}; }
    static constexpr Command next_item() { return {CommandType::NextItem}; }
    static constexpr Command use_item() { return {CommandType::UseItem}; }
    static constexpr Command give_up() { return {CommandType::GiveUp}; }

    friend constexpr bool operator==(Command a, Command b) {
        return a.type == b.type && (a.type != CommandType::Move || a.dir == b.dir);
    }
};

// Names used in recordings: "down", "up", "left", "right", "cw", "ccw", "next", "use", "giveup".
std::string_view command_name(Command c);
std::optional<Command> command_from_name(std::string_view name);

}  // namespace soldier
