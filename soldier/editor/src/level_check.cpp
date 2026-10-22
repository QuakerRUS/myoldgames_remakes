#include "level_check.hpp"

#include <array>
#include <queue>

#include "soldier/tile.hpp"

namespace soldier_editor {
namespace {

using soldier::Tile;

std::string cell_text(int x, int y) { return "(" + std::to_string(x) + ", " + std::to_string(y) + ")"; }

bool is_plain_door_code(int c) {
    if (c == static_cast<int>(Tile::Door) || c == static_cast<int>(Tile::BoxInDoor)) return true;
    const auto g = soldier::player_ground(static_cast<Tile>(c));
    if (g && *g == soldier::Ground::Door) return true;
    return c >= 21 && c <= 44 && (c - 21) % 4 == 3;  // key in door
}

bool is_button_code(int c) {
    if (c == static_cast<int>(Tile::Button) || c == static_cast<int>(Tile::BoxOnButton)) return true;
    const auto g = soldier::player_ground(static_cast<Tile>(c));
    if (g && *g == soldier::Ground::Button) return true;
    return c >= 21 && c <= 44 && (c - 21) % 4 == 2;  // key on button
}

// Cells the simple reachability flood fill cannot cross. Doors (plain and coloured) and boxes
// are treated as passable; walls, dark walls, water and inert decorations are not.
bool blocks_flood(int c) {
    const auto t = static_cast<Tile>(c);
    if (t == Tile::Wall || t == Tile::DarkWall || t == Tile::Water) return true;
    if (soldier::is_death(t)) return true;
    return soldier::is_player(t);  // other player tiles are decorations (the start is the origin)
}

}  // namespace

bool is_start_code(int c) {
    if (c < 0 || c >= soldier::kTileCount) return false;
    const auto g = soldier::player_ground(static_cast<Tile>(c));
    return g && *g != soldier::Ground::SunkBox;
}

int count_starts(const vs::TileGrid& grid) {
    int n = 0;
    for (int c : grid.cells()) n += is_start_code(c) ? 1 : 0;
    return n;
}

CellPos find_start(const vs::TileGrid& grid) {
    CellPos p;
    for (int y = 0; y < grid.height(); ++y)
        for (int x = 0; x < grid.width(); ++x)
            if (is_start_code(grid.at(x, y))) p = {x, y};
    return p;
}

CheckResult check_level(const vs::TileDocument& doc) {
    CheckResult r;
    const vs::TileGrid& g = doc.grid;

    int starts = 0, sx = -1, sy = -1, exits = 0, buttons = 0, plain_doors = 0;
    std::array<int, soldier::kKeyColorCount> keys{}, color_doors{};
    for (int y = 0; y < g.height(); ++y)
        for (int x = 0; x < g.width(); ++x) {
            const int c = g.at(x, y);
            if (is_start_code(c)) ++starts, sx = x, sy = y;  // the last one wins, as in the game
            if (c == static_cast<int>(Tile::Exit)) ++exits;
            if (is_button_code(c)) ++buttons;
            if (is_plain_door_code(c)) ++plain_doors;
            if (const auto kc = soldier::key_color(static_cast<Tile>(c))) {
                const auto i = static_cast<std::size_t>(*kc);
                if (c == static_cast<int>(soldier::color_door_tile(*kc)))
                    ++color_doors[i];
                else
                    ++keys[i];
            }
        }

    if (starts == 0) r.errors.push_back("No player start: place one from the \"Player start\" group.");
    if (starts > 1)
        r.warnings.push_back(std::to_string(starts - 1) + " extra player tile(s) are decorations; the start is the last one in "
                             "scan order, at " + cell_text(sx, sy) + ", as in the original.");
    if (exits == 0) r.errors.push_back("No exit: the level cannot be won.");

    if (starts > 0 && exits > 0) {
        std::vector<char> seen(g.cells().size(), 0);
        std::queue<std::pair<int, int>> q;
        q.push({sx, sy});
        seen[static_cast<std::size_t>(sy * g.width() + sx)] = 1;
        bool found = false;
        while (!q.empty() && !found) {
            const auto [x, y] = q.front();
            q.pop();
            constexpr std::array<std::pair<int, int>, 4> d = {{{1, 0}, {-1, 0}, {0, 1}, {0, -1}}};
            for (const auto& [dx, dy] : d) {
                const int nx = x + dx, ny = y + dy;
                if (!g.in_bounds(nx, ny)) continue;
                const auto i = static_cast<std::size_t>(ny * g.width() + nx);
                if (seen[i] || blocks_flood(g.at(nx, ny))) continue;
                if (g.at(nx, ny) == static_cast<int>(Tile::Exit)) found = true;
                seen[i] = 1;
                q.push({nx, ny});
            }
        }
        if (!found) r.warnings.push_back("No exit is reachable from the start (ignoring doors and boxes).");
    }

    if (plain_doors > 0 && buttons == 0)
        r.warnings.push_back("Doors without buttons: with no button on the level the doors are always open.");
    static constexpr std::array<const char*, 6> names = {"Red", "Green", "Blue", "Yellow", "White", "Black"};
    static constexpr std::array<const char*, 6> lower = {"red", "green", "blue", "yellow", "white", "black"};
    for (std::size_t i = 0; i < names.size(); ++i)
        if (color_doors[i] > 0 && keys[i] == 0)
            r.warnings.push_back(std::string(names[i]) + " door(s) without a " + lower[i] + " key: they cannot be opened.");
        else if (color_doors[i] > keys[i])
            r.warnings.push_back(std::string(names[i]) + ": " + std::to_string(color_doors[i]) + " doors but only " +
                                 std::to_string(keys[i]) + " key(s); each key opens one door.");
    return r;
}

}  // namespace soldier_editor
