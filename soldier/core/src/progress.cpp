#include "soldier/progress.hpp"

#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>

namespace soldier {
namespace {

std::optional<std::string> read_file(const std::filesystem::path& p) {
    std::ifstream in(p, std::ios::binary);
    if (!in) return std::nullopt;
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

std::string two_digits(int n) {
    char buf[16];
    std::snprintf(buf, sizeof buf, "%02d.json", n);
    return buf;
}

}  // namespace

bool record_proves_win(const Level& level, const Recording& rec, std::string_view player) {
    if (rec.level != level.number || rec.player != player) return false;
    if (!rec.result || rec.result->status != Status::Won) return false;
    try {
        const ReplayOutcome o = run_replay(level, rec);
        return o.status == Status::Won && o.tick == rec.result->tick;
    } catch (const std::exception&) {
        return false;  // e.g. a level without a start
    }
}

int verified_level_reached(int level_count, std::string_view player,
                           const std::function<std::optional<Level>(int)>& level_for,
                           const std::function<std::optional<Recording>(int)>& record_for) {
    int k = 0;
    while (k < level_count) {
        const int n = k + 1;
        const std::optional<Recording> rec = record_for(n);
        if (!rec) break;
        const std::optional<Level> lv = level_for(n);
        if (!lv || !record_proves_win(*lv, *rec, player)) break;
        k = n;
    }
    return k + 1;
}

int verified_level_reached(const std::filesystem::path& slot_dir, const std::filesystem::path& levels_dir,
                           int level_count, std::string_view player) {
    auto level_for = [&](int n) -> std::optional<Level> {
        const auto text = read_file(levels_dir / two_digits(n));
        if (!text) return std::nullopt;
        try {
            Level lv = parse_level_json(*text);
            lv.number = n;  // the file name decides, as in the game
            return lv;
        } catch (const std::exception&) {
            return std::nullopt;
        }
    };
    auto record_for = [&](int n) -> std::optional<Recording> {
        const auto text = read_file(slot_dir / two_digits(n));
        if (!text) return std::nullopt;
        try {
            return parse_recording_json(*text);
        } catch (const std::exception&) {
            return std::nullopt;
        }
    };
    return verified_level_reached(level_count, player, level_for, record_for);
}

}  // namespace soldier
