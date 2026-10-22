#include <array>

#include "soldier/command.hpp"
#include "soldier/event.hpp"

namespace soldier {

std::string_view command_name(Command c) {
    switch (c.type) {
    case CommandType::Move: return dir_name(c.dir);
    case CommandType::TurnCW: return "cw";
    case CommandType::TurnCCW: return "ccw";
    case CommandType::NextItem: return "next";
    case CommandType::UseItem: return "use";
    case CommandType::GiveUp: return "giveup";
    }
    return "?";
}

std::optional<Command> command_from_name(std::string_view n) {
    for (Dir d : {Dir::Down, Dir::Up, Dir::Left, Dir::Right})
        if (n == dir_name(d)) return Command::move(d);
    if (n == "cw") return Command::turn_cw();
    if (n == "ccw") return Command::turn_ccw();
    if (n == "next") return Command::next_item();
    if (n == "use") return Command::use_item();
    if (n == "giveup") return Command::give_up();
    return std::nullopt;
}

std::string_view event_name(EventType t) {
    static constexpr std::array<std::string_view, 20> names = {
        "moved",          "turned",          "bumped",          "box_pushed",         "box_sank",
        "button_pressed", "button_released", "doors_opened",    "doors_closed",       "key_picked",
        "key_dropped",    "color_door_unlocked", "wall_revealed", "fake_wall_entered", "item_selected",
        "guillotines_closed", "guillotines_opened", "timer_changed", "died",           "won",
    };
    const auto i = static_cast<std::size_t>(t);
    return i < names.size() ? names[i] : "?";
}

std::string_view death_cause_name(DeathCause c) {
    switch (c) {
    case DeathCause::GaveUp: return "gave_up";
    case DeathCause::TimeOut: return "time_out";
    case DeathCause::Guillotine: return "guillotine";
    case DeathCause::Drowned: return "drowned";
    }
    return "?";
}

}  // namespace soldier
