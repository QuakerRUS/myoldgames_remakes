#include "palette.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>

#include "soldier/tile.hpp"

namespace soldier_editor {
namespace {

using soldier::KeyColor;
using soldier::Tile;

constexpr std::array<const char*, 6> kColorNames = {"Red", "Green", "Blue", "Yellow", "White", "Black"};
constexpr std::array<const char*, 4> kDirNames = {"down", "up", "left", "right"};

int code(Tile t) { return static_cast<int>(t); }

std::vector<PaletteGroup> build_groups() {
    std::vector<PaletteGroup> g;
    g.push_back({"Basic", {{code(Tile::Floor), "Floor"},
                           {code(Tile::Wall), "Wall"},
                           {code(Tile::Exit), "Exit"},
                           {code(Tile::DarkWall), "Dark wall"},
                           {code(Tile::FakeWall), "Fake wall"}}});
    g.push_back({"Player start", {{code(Tile::PlayerDown), "Player start (down)"},
                                  {code(Tile::PlayerUp), "Player start (up)"},
                                  {code(Tile::PlayerLeft), "Player start (left)"},
                                  {code(Tile::PlayerRight), "Player start (right)"}}});
    g.push_back({"Boxes, buttons and doors", {{code(Tile::Box), "Box"},
                                              {code(Tile::Button), "Button"},
                                              {code(Tile::BoxOnButton), "Box on button"},
                                              {code(Tile::Door), "Door"},
                                              {code(Tile::BoxInDoor), "Box in door"}}});
    g.push_back({"Hazards and water", {{code(Tile::Guillotine), "Guillotine"},
                                       {code(Tile::Water), "Water"},
                                       {code(Tile::SunkBox), "Sunk box"},
                                       {code(Tile::BoxOnSunkBox), "Box on sunk box"}}});
    PaletteGroup keys{"Keys and coloured doors", {}};
    for (int c = 0; c < soldier::kKeyColorCount; ++c) {
        const auto kc = static_cast<KeyColor>(c);
        const std::string n = kColorNames[static_cast<std::size_t>(c)];
        keys.entries.push_back({code(soldier::key_tile(kc)), n + " key"});
        keys.entries.push_back({code(soldier::color_door_tile(kc)), n + " door"});
        keys.entries.push_back({code(soldier::key_on_button_tile(kc)), n + " key on button"});
        keys.entries.push_back({code(soldier::key_in_door_tile(kc)), n + " key in door"});
    }
    g.push_back(std::move(keys));
    return g;
}

}  // namespace

const std::vector<PaletteGroup>& palette_groups() {
    static const std::vector<PaletteGroup> groups = build_groups();
    return groups;
}

std::vector<PaletteEntry> flat_palette() {
    std::vector<PaletteEntry> out;
    for (const PaletteGroup& g : palette_groups()) out.insert(out.end(), g.entries.begin(), g.entries.end());
    return out;
}

std::string tile_display_name(int c) {
    if (c < 0 || c >= soldier::kTileCount) return "Unknown";
    const auto t = static_cast<Tile>(c);
    if (const auto ground = soldier::player_ground(t)) {
        const char* dir = kDirNames[static_cast<std::size_t>(*soldier::player_facing(t))];
        switch (*ground) {
        case soldier::Ground::Floor: return std::string("Player start (") + dir + ")";
        case soldier::Ground::Button: return std::string("Player on button (") + dir + ")";
        case soldier::Ground::Door: return std::string("Player in door (") + dir + ")";
        case soldier::Ground::Guillotine: return std::string("Player under guillotine (") + dir + ")";
        case soldier::Ground::SunkBox: return std::string("Player on sunk box (") + dir + ", decoration)";
        }
    }
    if (const auto kc = soldier::key_color(t)) {
        const std::string n = kColorNames[static_cast<std::size_t>(*kc)];
        switch ((c - 21) % 4) {
        case 0: return n + " key";
        case 1: return n + " door";
        case 2: return n + " key on button";
        default: return n + " key in door";
        }
    }
    switch (t) {
    case Tile::Floor: return "Floor";
    case Tile::Wall: return "Wall";
    case Tile::Exit: return "Exit";
    case Tile::DeathGaveUp: return "Death: gave up (decoration)";
    case Tile::Box: return "Box";
    case Tile::Button: return "Button";
    case Tile::BoxOnButton: return "Box on button";
    case Tile::Door: return "Door";
    case Tile::BoxInDoor: return "Box in door";
    case Tile::DarkWall: return "Dark wall";
    case Tile::FakeWall: return "Fake wall";
    case Tile::DeathTimeOut: return "Death: time out (decoration)";
    case Tile::Guillotine: return "Guillotine";
    case Tile::DeathGuillotine: return "Death: guillotine (decoration)";
    case Tile::Water: return "Water";
    case Tile::DeathDrowned: return "Death: drowned (decoration)";
    case Tile::SunkBox: return "Sunk box";
    case Tile::DeathGaveUpOnSunkBox: return "Death: gave up on sunk box (decoration)";
    case Tile::DeathTimeOutOnSunkBox: return "Death: time out on sunk box (decoration)";
    case Tile::BoxOnSunkBox: return "Box on sunk box";
    default: return "Unknown";
    }
}

std::string_view tile_hint(int c) {
    if (c < 0 || c >= soldier::kTileCount) return "Unknown tile code.";
    const auto t = static_cast<Tile>(c);
    if (soldier::is_death(t))
        return "Picture of a dead soldier left by the original levels. Inert: the player cannot enter it.";
    if (const auto ground = soldier::player_ground(t)) {
        if (*ground == soldier::Ground::SunkBox)
            return "Player standing on a sunk box. Never a start: an inert decoration.";
        return "The soldier's start, facing this way. Placing it moves the current start and keeps the ground "
               "(button, door, guillotine). If several player tiles exist, the game starts at the last one in "
               "reading order; the others are decorations.";
    }
    if (soldier::key_color(t)) {
        switch ((c - 21) % 4) {
        case 0: return "A key: walking onto it picks it up (25 keys at most). Opens one door of its colour.";
        case 1: return "Coloured door: opens (and is consumed with one key) when the player walks into it "
                       "carrying a key of the same colour.";
        case 2: return "A key lying on a button: the button stays pressed until the key is picked up.";
        default: return "A key lying in a plain door; it can be picked up even when the door is closed.";
        }
    }
    switch (t) {
    case Tile::Floor: return "Empty floor. Backspace and the right mouse button reset cells to floor.";
    case Tile::Wall: return "Solid brick wall. Outside the level also behaves as a wall.";
    case Tile::Exit: return "Reaching it wins the level. At least one is required.";
    case Tile::Box: return "Pushable crate: moves onto floor, a button (pressing it), an open door, or into water "
                           "(sinks and becomes a sunk box).";
    case Tile::Button: return "Pressure plate. Plain doors are open only while every button is pressed by the "
                              "player, a box or a key.";
    case Tile::BoxOnButton: return "A crate pressing a button. Pushing it away releases the button.";
    case Tile::Door: return "Plain door: open while every button on the level is pressed (always open if there "
                            "are no buttons).";
    case Tile::BoxInDoor: return "A crate standing in a plain door; it can be pushed out even when the door is closed.";
    case Tile::DarkWall: return "Dark wall: turns into a normal brick wall the first time the player bumps into it.";
    case Tile::FakeWall: return "Fake wall: looks exactly like a dark wall, but the player walks through it "
                                "(it becomes floor). Marked with '?' in the editor only.";
    case Tile::Guillotine: return "Closes during the last quarter of every game second: the player under it then "
                                  "dies, and cannot enter it while it is closed.";
    case Tile::Water: return "Walking in drowns the player. A box pushed in sinks and becomes a sunk box.";
    case Tile::SunkBox: return "A crate sunk in water: a walkable bridge. Another box can be pushed onto it.";
    case Tile::BoxOnSunkBox: return "A crate standing on a sunk box; pushing it away leaves the sunk box.";
    default: return "";
    }
}

std::vector<int> palette_group_sizes() {
    std::vector<int> sizes;
    for (const PaletteGroup& g : palette_groups()) sizes.push_back(static_cast<int>(g.entries.size()));
    return sizes;
}

std::vector<PaletteCell> palette_layout(int columns) { return vs::palette_layout(palette_group_sizes(), columns); }

int palette_move(int index, int dx, int dy, int columns) {
    return vs::palette_move(palette_group_sizes(), columns, index, dx, dy, true);
}

}  // namespace soldier_editor
