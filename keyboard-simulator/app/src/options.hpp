#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace ks {

// Debug / automation flags (besides the engine's --smoke, --windowed, --fullscreen).
//   --screenshot <png>   render a frame, save it and quit
//   --autotype <n>       type the first n expected characters correctly (before the screenshot)
//   --type "<utf8>"      then type this text verbatim (wrong characters count as errors)
//   --state paused|help|about|quit   open this state before the screenshot
//   --seed <n>           fixed sentence order
struct Options {
    std::string screenshot;
    int autotype = 0;
    std::string type;
    std::string state;
    std::optional<std::uint64_t> seed;

    bool scripted() const { return !screenshot.empty() || autotype > 0 || !type.empty() || !state.empty(); }
};

Options parse_options(const std::vector<std::string>& args);

}  // namespace ks
