#pragma once
// Score rules: 10 x level per rabbit, 500 x level for completing a level.

namespace piton_game {

inline constexpr double kTickSeconds = 0.125;  // 8 cells per second

constexpr int rabbit_points(int level) { return 10 * level; }
constexpr int level_bonus(int level) { return 500 * level; }

}  // namespace piton_game
