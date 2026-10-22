// Tile codes of Soldier (identical to the byte values of the original LEVELn.DAT files).
#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

namespace soldier {

enum class Tile : std::uint8_t {
    Floor = 0,
    Wall = 1,
    Exit = 2,
    PlayerDown = 3, PlayerUp = 4, PlayerLeft = 5, PlayerRight = 6,  // player on floor
    DeathGaveUp = 7,
    Box = 8,
    Button = 9,
    BoxOnButton = 10,
    PlayerOnButtonDown = 11, PlayerOnButtonUp = 12, PlayerOnButtonLeft = 13, PlayerOnButtonRight = 14,
    Door = 15,
    PlayerInDoorDown = 16, PlayerInDoorUp = 17, PlayerInDoorLeft = 18, PlayerInDoorRight = 19,
    BoxInDoor = 20,
    RedKey = 21, RedDoor = 22, RedKeyOnButton = 23, RedKeyInDoor = 24,
    GreenKey = 25, GreenDoor = 26, GreenKeyOnButton = 27, GreenKeyInDoor = 28,
    BlueKey = 29, BlueDoor = 30, BlueKeyOnButton = 31, BlueKeyInDoor = 32,
    YellowKey = 33, YellowDoor = 34, YellowKeyOnButton = 35, YellowKeyInDoor = 36,
    WhiteKey = 37, WhiteDoor = 38, WhiteKeyOnButton = 39, WhiteKeyInDoor = 40,
    BlackKey = 41, BlackDoor = 42, BlackKeyOnButton = 43, BlackKeyInDoor = 44,
    DarkWall = 45,
    FakeWall = 46,
    DeathTimeOut = 47,
    Guillotine = 48,
    PlayerInGuillotineDown = 49, PlayerInGuillotineUp = 50, PlayerInGuillotineLeft = 51, PlayerInGuillotineRight = 52,
    DeathGuillotine = 53,
    Water = 54,
    DeathDrowned = 55,
    SunkBox = 56,
    PlayerOnSunkBoxDown = 57, PlayerOnSunkBoxUp = 58, PlayerOnSunkBoxLeft = 59, PlayerOnSunkBoxRight = 60,
    DeathGaveUpOnSunkBox = 61,
    DeathTimeOutOnSunkBox = 62,
    BoxOnSunkBox = 63,
};

inline constexpr int kTileCount = 64;

// Facing / step direction. The order matches the original (n = 1..4 is Down, Up, Left, Right).
enum class Dir : std::uint8_t { Down = 0, Up = 1, Left = 2, Right = 3 };

struct Delta { int dx, dy; };
constexpr Delta delta(Dir d) {
    switch (d) {
    case Dir::Down: return {0, 1};
    case Dir::Up: return {0, -1};
    case Dir::Left: return {-1, 0};
    case Dir::Right: return {1, 0};
    }
    return {0, 0};
}
Dir turn_cw(Dir d);   // Down -> Left -> Up -> Right -> Down (the original "." / ">")
Dir turn_ccw(Dir d);  // Down -> Right -> Up -> Left -> Down (the original "," / "<")

// The six key/door colours, in code order (Red = 21..24, ..., Black = 41..44).
enum class KeyColor : std::uint8_t { Red = 0, Green, Blue, Yellow, White, Black };
inline constexpr int kKeyColorCount = 6;

// What lies under the player. The player tile code is ground base + facing.
enum class Ground : std::uint8_t { Floor, Button, Door, Guillotine, SunkBox };

bool is_player(Tile t);
std::optional<Ground> player_ground(Tile t);  // nullopt if t is not a player tile
std::optional<Dir> player_facing(Tile t);
Tile player_tile(Ground g, Dir d);
Tile ground_tile(Ground g);  // Floor, Button, Door, Guillotine, SunkBox

bool is_death(Tile t);  // 7, 47, 53, 55, 61, 62

// Coloured key/door helpers (codes 21..44).
std::optional<KeyColor> key_color(Tile t);  // colour of any of the 4 coloured codes
Tile key_tile(KeyColor c);                  // key lying on the floor
Tile color_door_tile(KeyColor c);
Tile key_on_button_tile(KeyColor c);
Tile key_in_door_tile(KeyColor c);

std::string_view tile_name(Tile t);         // e.g. "box_on_button"
std::string_view key_color_name(KeyColor c);  // "red", ...
std::string_view dir_name(Dir d);           // "down", "up", "left", "right"

// One-character form used in the level JSON files (see docs/formats/soldier-level.md).
char tile_char(Tile t);
std::optional<Tile> tile_from_char(char c);

}  // namespace soldier
