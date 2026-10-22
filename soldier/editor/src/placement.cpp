#include "placement.hpp"

#include <algorithm>

#include "level_check.hpp"
#include "soldier/level.hpp"
#include "soldier/tile.hpp"

namespace soldier_editor {

using soldier::Ground;
using soldier::Tile;

std::vector<vs::CellChange> placement_changes(const vs::TileDocument& doc, int x, int y, int code) {
    const vs::TileGrid& g = doc.grid;
    if (!g.in_bounds(x, y) || code < 0 || code >= soldier::kTileCount) return {};
    if (!is_start_code(code)) return {{x, y, code}};

    const auto dir = *soldier::player_facing(static_cast<Tile>(code));
    Ground ground = *soldier::player_ground(static_cast<Tile>(code));
    const auto here = static_cast<Tile>(g.at(x, y));
    if (here == Tile::Button) ground = Ground::Button;
    else if (here == Tile::Door) ground = Ground::Door;
    else if (here == Tile::Guillotine) ground = Ground::Guillotine;
    else if (is_start_code(g.at(x, y))) ground = *soldier::player_ground(here);

    std::vector<vs::CellChange> out{{x, y, static_cast<int>(soldier::player_tile(ground, dir))}};
    const CellPos old = find_start(g);  // moved; the other player tiles stay as decorations
    if (old.x >= 0 && (old.x != x || old.y != y))
        out.push_back({old.x, old.y, static_cast<int>(soldier::ground_tile(*soldier::player_ground(static_cast<Tile>(g.at(old.x, old.y)))))});
    return out;
}

std::string start_placement_warning(const vs::TileDocument& doc, int x, int y) {
    const CellPos s = find_start(doc.grid);
    if (s.x < 0 || (s.x == x && s.y == y)) return {};
    return "Warning: the start is the last player tile in row-major order, at (" + std::to_string(s.x) + ", " +
           std::to_string(s.y) + "); the tile placed at (" + std::to_string(x) + ", " + std::to_string(y) +
           ") is a decoration.";
}

vs::TileDocument resized(const vs::TileDocument& doc, int width, int height) {
    vs::TileDocument out = doc;
    out.grid.resize(std::clamp(width, 1, soldier::kMaxLevelSize), std::clamp(height, 1, soldier::kMaxLevelSize),
                    static_cast<int>(Tile::Floor));
    return out;
}

}  // namespace soldier_editor
