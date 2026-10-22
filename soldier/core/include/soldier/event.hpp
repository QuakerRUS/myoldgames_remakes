// Events emitted by soldier::Game for the presentation layer (animation and sound).
#pragma once

#include <cstdint>
#include <string_view>

#include "soldier/level.hpp"
#include "soldier/tile.hpp"

namespace soldier {

enum class DeathCause : std::uint8_t { GaveUp, TimeOut, Guillotine, Drowned };

enum class EventType : std::uint8_t {
    Moved,              // player stepped: at = from, to = new cell, dir
    Turned,             // facing changed (turn command or bump): dir = new facing
    Bumped,             // move blocked: at = blocked cell, dir = attempted direction
    BoxPushed,          // at = old box cell, to = new box cell, dir
    BoxSank,            // at = water cell that became a sunk box (follows BoxPushed)
    ButtonPressed,      // at = button cell (by the player, a box or a key)
    ButtonReleased,     // at = button cell (the player left it)
    DoorsOpened,        // every button is now pressed: all plain doors (15) are open
    DoorsClosed,        // a button was released: plain doors are closed again
    KeyPicked,          // at = cell, color; the key was appended to the inventory
    KeyDropped,         // at = cell, color, tile = resulting tile (key / key on button / key in door)
    ColorDoorUnlocked,  // at = door cell (now floor), color; the key was consumed
    WallRevealed,       // at = dark wall cell that turned into a plain wall
    FakeWallEntered,    // at = fake wall cell the player walked into (it becomes floor when left)
    ItemSelected,       // value = new inventory cursor (0-based)
    GuillotinesClosed,  // the 4th part of a game second begins
    GuillotinesOpened,  // the 1st part of a game second begins
    TimerChanged,       // value = new timer value
    Died,               // at = player cell, cause, tile = death tile drawn there
    Won,                // at = exit cell
};

struct Event {
    EventType type = EventType::Moved;
    Point at{};
    Point to{};
    Dir dir = Dir::Down;
    KeyColor color = KeyColor::Red;
    DeathCause cause = DeathCause::GaveUp;
    Tile tile = Tile::Floor;
    int value = 0;
};

std::string_view event_name(EventType t);
std::string_view death_cause_name(DeathCause c);  // "gave_up", "time_out", "guillotine", "drowned"

}  // namespace soldier
