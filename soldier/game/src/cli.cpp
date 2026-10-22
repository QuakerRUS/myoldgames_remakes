#include "cli.hpp"

#include <cstdlib>

namespace sg {

CommandLine parse_command_line(const std::vector<std::string>& args) {
    CommandLine cl;
    const std::size_t n = args.size();
    auto value = [&](std::size_t& i) -> std::string {
        if (i + 1 < n) return args[++i];
        return {};
    };
    for (std::size_t i = 0; i < n; ++i) {
        const std::string& a = args[i];
        if (a == "--test-level")
            cl.test_level = value(i);
        else if (a == "--verify-replays")
            cl.verify_replays = true;
        else if (a == "--scene")
            cl.scene = value(i);
        else if (a == "--level")
            cl.level = std::atoi(value(i).c_str());
        else if (a == "--record")
            cl.record = value(i);
        else if (a == "--screenshot")
            cl.screenshot = value(i);
        else if (a == "--shot-time")
            cl.shot_time = std::atof(value(i).c_str());
        else if (a == "--shot-tick")
            cl.shot_tick = std::atoi(value(i).c_str());
        else if (a == "--shot-delay")
            cl.shot_delay = std::atof(value(i).c_str());
        else if (a == "--time-scale")
            cl.time_scale = std::atof(value(i).c_str());
        else if (a == "--autoplay-record")
            cl.autoplay_record = true;
        else if (a == "--give-up-at")
            cl.give_up_at = std::atoi(value(i).c_str());
        else if (a == "--menu-open")
            cl.menu_open = value(i);
        else if (a == "--log-events")
            cl.log_events = true;
        else if (a == "--script")
            cl.script = value(i);
        else if (a == "--replay-speed")
            cl.replay_speed = std::atof(value(i).c_str());
        else
            cl.unknown.push_back(a);
    }
    if (cl.time_scale <= 0) cl.time_scale = 1.0;
    return cl;
}

}  // namespace sg
