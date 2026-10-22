#include "launch_options.hpp"

#include <algorithm>
#include <cstdlib>

namespace piton_game {

LaunchOptions parse_launch_options(const std::vector<std::string>& rest) {
    LaunchOptions o;
    for (std::size_t i = 0; i < rest.size(); ++i) {
        const std::string& a = rest[i];
        const bool has_value = i + 1 < rest.size();
        if (a == "--test-level" && has_value) {
            o.test_level = rest[++i];
        } else if (a == "--shot-scene" && has_value) {
            o.shot.scene = rest[++i];
        } else if (a == "--shot-out" && has_value) {
            o.shot.out = rest[++i];
        } else if (a == "--shot-level" && has_value) {
            o.shot.level = std::atoi(rest[++i].c_str());
        } else if (a == "--shot-ticks" && has_value) {
            o.shot.ticks = std::atoi(rest[++i].c_str());
        } else if (a == "--shot-frames" && has_value) {
            o.shot.frames = std::max(1, std::atoi(rest[++i].c_str()));
        } else if (a == "--shot-until-turn" && has_value) {
            o.shot.until_turn = rest[++i];
        } else if (a == "--shot-time" && has_value) {
            o.shot.time = std::atof(rest[++i].c_str());
        }
    }
    if (o.shot.level < 1) o.shot.level = 1;
    if (o.shot.level > 10) o.shot.level = 10;
    return o;
}

LaunchOptions& launch_options() {
    static LaunchOptions options;
    return options;
}

}  // namespace piton_game
