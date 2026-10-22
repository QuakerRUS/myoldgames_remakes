#include "soldier/tile.hpp"

#include <array>

namespace soldier {
namespace {

// Index = tile code. Must stay in sync with docs/formats/soldier-level.md and tools/import/soldier_levels.py.
constexpr std::array<char, kTileCount> kTileChars = {
    '.', '#', 'X', 'v', '^', '<', '>', '!', 'B', '_', 'b',  // 0..10
    '1', '2', '3', '4', 'D', '5', '6', '7', '8', 'd',       // 11..20
    'r', 'R', 'a', 'A',                                     // red 21..24
    'g', 'G', 'h', 'H',                                     // green
    'u', 'U', 'i', 'I',                                     // blue
    'y', 'Y', 'j', 'J',                                     // yellow
    'w', 'W', 'l', 'L',                                     // white
    'k', 'K', 'm', 'M',                                     // black
    '%', '?', '@', 'T', '9', '0', '{', '}', '$', '~', '*',  // 45..55
    '=', '(', ')', '[', ']', '-', '+', '&',                 // 56..63
};

constexpr std::array<std::string_view, kTileCount> kTileNames = {
    "floor", "wall", "exit",
    "player_down", "player_up", "player_left", "player_right",
    "death_gave_up", "box", "button", "box_on_button",
    "player_on_button_down", "player_on_button_up", "player_on_button_left", "player_on_button_right",
    "door",
    "player_in_door_down", "player_in_door_up", "player_in_door_left", "player_in_door_right",
    "box_in_door",
    "red_key", "red_door", "red_key_on_button", "red_key_in_door",
    "green_key", "green_door", "green_key_on_button", "green_key_in_door",
    "blue_key", "blue_door", "blue_key_on_button", "blue_key_in_door",
    "yellow_key", "yellow_door", "yellow_key_on_button", "yellow_key_in_door",
    "white_key", "white_door", "white_key_on_button", "white_key_in_door",
    "black_key", "black_door", "black_key_on_button", "black_key_in_door",
    "dark_wall", "fake_wall", "death_time_out", "guillotine",
    "player_in_guillotine_down", "player_in_guillotine_up", "player_in_guillotine_left",
    "player_in_guillotine_right",
    "death_guillotine", "water", "death_drowned", "sunk_box",
    "player_on_sunk_box_down", "player_on_sunk_box_up", "player_on_sunk_box_left",
    "player_on_sunk_box_right",
    "death_gave_up_on_sunk_box", "death_time_out_on_sunk_box", "box_on_sunk_box",
};

constexpr std::array<int, 5> kGroundBase = {3, 11, 16, 49, 57};
constexpr std::array<Tile, 5> kGroundTile = {Tile::Floor, Tile::Button, Tile::Door, Tile::Guillotine,
                                             Tile::SunkBox};

int code(Tile t) { return static_cast<int>(t); }

}  // namespace

Dir turn_cw(Dir d) {
    switch (d) {
    case Dir::Down: return Dir::Left;
    case Dir::Left: return Dir::Up;
    case Dir::Up: return Dir::Right;
    case Dir::Right: return Dir::Down;
    }
    return d;
}

Dir turn_ccw(Dir d) {
    switch (d) {
    case Dir::Down: return Dir::Right;
    case Dir::Right: return Dir::Up;
    case Dir::Up: return Dir::Left;
    case Dir::Left: return Dir::Down;
    }
    return d;
}

std::optional<Ground> player_ground(Tile t) {
    const int c = code(t);
    for (std::size_t i = 0; i < kGroundBase.size(); ++i)
        if (c >= kGroundBase[i] && c < kGroundBase[i] + 4) return static_cast<Ground>(i);
    return std::nullopt;
}

bool is_player(Tile t) { return player_ground(t).has_value(); }

std::optional<Dir> player_facing(Tile t) {
    const auto g = player_ground(t);
    if (!g) return std::nullopt;
    return static_cast<Dir>(code(t) - kGroundBase[static_cast<int>(*g)]);
}

Tile player_tile(Ground g, Dir d) {
    return static_cast<Tile>(kGroundBase[static_cast<int>(g)] + static_cast<int>(d));
}

Tile ground_tile(Ground g) { return kGroundTile[static_cast<int>(g)]; }

bool is_death(Tile t) {
    switch (t) {
    case Tile::DeathGaveUp:
    case Tile::DeathTimeOut:
    case Tile::DeathGuillotine:
    case Tile::DeathDrowned:
    case Tile::DeathGaveUpOnSunkBox:
    case Tile::DeathTimeOutOnSunkBox: return true;
    default: return false;
    }
}

std::optional<KeyColor> key_color(Tile t) {
    const int c = code(t);
    if (c < 21 || c > 44) return std::nullopt;
    return static_cast<KeyColor>((c - 21) / 4);
}

Tile key_tile(KeyColor c) { return static_cast<Tile>(21 + 4 * static_cast<int>(c)); }
Tile color_door_tile(KeyColor c) { return static_cast<Tile>(22 + 4 * static_cast<int>(c)); }
Tile key_on_button_tile(KeyColor c) { return static_cast<Tile>(23 + 4 * static_cast<int>(c)); }
Tile key_in_door_tile(KeyColor c) { return static_cast<Tile>(24 + 4 * static_cast<int>(c)); }

std::string_view tile_name(Tile t) {
    const int c = code(t);
    return c < kTileCount ? kTileNames[c] : std::string_view("unknown");
}

std::string_view key_color_name(KeyColor c) {
    static constexpr std::array<std::string_view, kKeyColorCount> names = {"red",   "green", "blue",
                                                                           "yellow", "white", "black"};
    return names[static_cast<int>(c)];
}

std::string_view dir_name(Dir d) {
    static constexpr std::array<std::string_view, 4> names = {"down", "up", "left", "right"};
    return names[static_cast<int>(d)];
}

char tile_char(Tile t) {
    const int c = code(t);
    return c < kTileCount ? kTileChars[c] : '\0';
}

std::optional<Tile> tile_from_char(char ch) {
    for (int i = 0; i < kTileCount; ++i)
        if (kTileChars[i] == ch) return static_cast<Tile>(i);
    return std::nullopt;
}

}  // namespace soldier
