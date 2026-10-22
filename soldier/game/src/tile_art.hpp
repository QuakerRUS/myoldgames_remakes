// How a tile code is drawn: decomposition into a ground layer and an object on it, the
// autotile masks and a few animation-time helpers. SDL-free.
#pragma once

#include <optional>
#include <string>

#include <vs/engine/sprite_sheet_data.hpp>
#include <soldier/game.hpp>

namespace sg {

enum class GroundArt : unsigned char {
    Floor, Wall, DarkWall, Exit, Button, Door, ColorDoor, Guillotine, Water, SunkBox,
};

enum class ObjectArt : unsigned char { None, Box, Key, Player, Death };

struct TileArt {
    GroundArt ground = GroundArt::Floor;
    ObjectArt object = ObjectArt::None;
    soldier::KeyColor color = soldier::KeyColor::Red;  // ColorDoor / Key
    soldier::Dir facing = soldier::Dir::Down;          // Player
    soldier::Tile death = soldier::Tile::Floor;        // Death: the death code
};

TileArt tile_art(soldier::Tile t);
// Ground layer of what the player stands on.
GroundArt ground_art(soldier::Ground g);
// tile_art() of the cell where the player died: the death tiles 7 / 47 carry no ground, so
// the ground the player stood on (button, door, ...) is kept under the death animation.
TileArt death_cell_art(soldier::Tile t, soldier::Ground ground);

bool is_wall_like(soldier::Tile t);       // 1, 45, 46 (blocks light: shadows)
bool is_water_like(soldier::Tile t);      // water, sunk box and things lying in water
char dir_letter(soldier::Dir d);          // 'd', 'u', 'l', 'r'
const char* color_name(soldier::KeyColor c);

// Name of the frame an animation shows `ms` milliseconds after its start (non-looping
// animations stay on their last frame). Empty if def is null.
std::string frame_at(const vs::AnimationDef* def, double ms);
// Total length in milliseconds (0 if null).
double anim_length(const vs::AnimationDef* def);

// Cheap deterministic hash of a cell (floor variants, shimmer offsets).
unsigned cell_hash(int x, int y);

}  // namespace sg
