#include <vs/editor/palette_grid.hpp>

#include <algorithm>
#include <cstdlib>
#include <numeric>

namespace vs {

std::vector<PaletteCell> palette_layout(const std::vector<int>& group_sizes, int columns) {
    columns = std::max(columns, 1);
    std::vector<PaletteCell> out;
    int row = 0;
    for (const int n : group_sizes) {
        for (int i = 0; i < n; ++i) out.push_back({row + i / columns, i % columns});
        row += (n + columns - 1) / columns;
    }
    return out;
}

int palette_move(const std::vector<int>& group_sizes, int columns, int index, int dx, int dy, bool wrap) {
    const int n = std::accumulate(group_sizes.begin(), group_sizes.end(), 0);
    if (n <= 0) return 0;
    columns = std::max(columns, 1);
    if (!wrap) return std::clamp(index + dx + dy * columns, 0, n - 1);
    const std::vector<PaletteCell> cells = palette_layout(group_sizes, columns);
    index = std::clamp(index, 0, n - 1);
    if (dx != 0) index = ((index + dx) % n + n) % n;
    if (dy != 0) {
        const int rows = cells.back().row + 1;
        const PaletteCell cur = cells[static_cast<std::size_t>(index)];
        const int target_row = ((cur.row + dy) % rows + rows) % rows;
        int best = index;
        int best_dist = 1 << 30;
        for (int i = 0; i < n; ++i) {
            const PaletteCell& c = cells[static_cast<std::size_t>(i)];
            if (c.row != target_row) continue;
            const int d = std::abs(c.col - cur.col);
            if (d < best_dist) best = i, best_dist = d;
        }
        index = best;
    }
    return index;
}

}  // namespace vs
