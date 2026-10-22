#pragma once
// Draws a level's ground: grass variants, brick autotiles and the shade they cast on the grass.
// The tile choice is computed once per level (build()), drawing is a plain loop.

#include <string>
#include <vector>

#include <vs/engine/renderer.hpp>
#include <vs/engine/sprite_sheet.hpp>

#include <piton/level.hpp>

namespace piton_game {

inline constexpr float kCell = 24.0f;  // pixels per cell
inline constexpr float kFieldW = kCell * piton::kFieldWidth;   // 912
inline constexpr float kFieldH = kCell * piton::kFieldHeight;  // 552

class FieldRenderer {
public:
    // Chooses the frames for every cell. `seed` varies the grass pattern between levels.
    void build(const piton::Level& level, unsigned seed);
    // Draws the field with its top-left corner at `origin`.
    void draw(vs::Renderer& r, const vs::SpriteSheet& tiles, vs::Vec2 origin) const;

private:
    struct CellFrames {
        std::string base;                 // grass_N or wall_N
        std::vector<std::string> shades;  // shade_n / shade_w / shade_nw overlays
    };
    std::vector<CellFrames> cells_;
};

}  // namespace piton_game
