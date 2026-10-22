#include "field_renderer.hpp"

namespace piton_game {
namespace {

// Cheap integer hash for stable per-cell variation.
unsigned hash_cell(int x, int y, unsigned seed) {
    unsigned h = static_cast<unsigned>(x) * 73856093u ^ static_cast<unsigned>(y) * 19349663u ^ seed * 83492791u;
    h ^= h >> 13;
    h *= 0x5bd1e995u;
    h ^= h >> 15;
    return h;
}

std::string grass_frame(int x, int y, unsigned seed) {
    const unsigned h = hash_cell(x, y, seed) % 100u;
    if (h < 3) return "grass_6";  // tiny flowers, rare
    if (h < 6) return "grass_7";
    return "grass_" + std::to_string(hash_cell(y, x, seed + 7) % 6u);
}

}  // namespace

void FieldRenderer::build(const piton::Level& level, unsigned seed) {
    cells_.assign(piton::kCellCount, {});
    // Outside the field counts as "no wall" so walls at the edge get their outline.
    auto wall = [&](int x, int y) { return piton::insideField({x, y}) && level.isBrick({x, y}); };
    for (int y = 0; y < piton::kFieldHeight; ++y) {
        for (int x = 0; x < piton::kFieldWidth; ++x) {
            CellFrames& c = cells_[static_cast<std::size_t>(piton::cellIndex({x, y}))];
            if (wall(x, y)) {
                const int mask = (wall(x, y - 1) ? 1 : 0) | (wall(x + 1, y) ? 2 : 0) | (wall(x, y + 1) ? 4 : 0) |
                                 (wall(x - 1, y) ? 8 : 0);
                c.base = "wall_" + std::to_string(mask);
                continue;
            }
            c.base = grass_frame(x, y, seed);
            const bool n = wall(x, y - 1);
            const bool w = wall(x - 1, y);
            if (n) c.shades.push_back("shade_n");
            if (w) c.shades.push_back("shade_w");
            if (!n && !w && wall(x - 1, y - 1)) c.shades.push_back("shade_nw");
        }
    }
}

void FieldRenderer::draw(vs::Renderer& r, const vs::SpriteSheet& tiles, vs::Vec2 origin) const {
    if (cells_.empty()) return;
    for (int i = 0; i < piton::kCellCount; ++i) {
        const piton::Cell c = piton::cellAt(i);
        const vs::Vec2 p{origin.x + static_cast<float>(c.x) * kCell, origin.y + static_cast<float>(c.y) * kCell};
        const CellFrames& f = cells_[static_cast<std::size_t>(i)];
        tiles.draw(r, f.base, p);
        for (const std::string& s : f.shades) tiles.draw(r, s, p);
    }
}

}  // namespace piton_game
