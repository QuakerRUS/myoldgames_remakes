// Progress proven by records: a player's "level reached" is derived from his winning records,
// so editing save.json cannot unlock levels (the original hung the computer on a tampered save;
// the remake quietly uses the verified value instead).
#pragma once

#include <filesystem>
#include <functional>
#include <optional>
#include <string_view>

#include "soldier/level.hpp"
#include "soldier/recording.hpp"

namespace soldier {

// True if `rec` proves that `player` won `level`: it is for that level number and that player,
// its result is a win, and replaying it with the rules ends in a win at exactly its recorded
// result tick.
bool record_proves_win(const Level& level, const Recording& rec, std::string_view player);

// 1 + the largest k such that levels 1..k all have a record proving a win by `player`
// (at most level_count + 1 = everything completed). `level_for(n)` / `record_for(n)` return
// std::nullopt when the level or the record is missing or invalid.
int verified_level_reached(int level_count, std::string_view player,
                           const std::function<std::optional<Level>(int)>& level_for,
                           const std::function<std::optional<Recording>(int)>& record_for);

// The same for records stored as <slot_dir>/NN.json and levels as <levels_dir>/NN.json.
int verified_level_reached(const std::filesystem::path& slot_dir, const std::filesystem::path& levels_dir,
                           int level_count, std::string_view player);

}  // namespace soldier
