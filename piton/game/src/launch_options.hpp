#pragma once
// Command-line options of piton.exe beyond the engine's common ones (--smoke, --windowed,
// --fullscreen).
//
//   --test-level <path.json>   play that level directly (map editor F5); the program exits
//                              after game over, level complete or Esc.
//
// Debug screenshots (automated visual checks; no input needed, the program quits itself):
//   --shot-scene <name>        intro | menu | options | levels | game | demo | pause | quit |
//                              gameover | complete | win
//   --shot-out <file.png>      output file; with --shot-frames N > 1: file-0.png ... file-(N-1).png
//   --shot-level <n>           level for the game scenes (default 1)
//   --shot-ticks <n>           ticks the demo pilot plays before the shot (default 0)
//   --shot-frames <n>          consecutive sub-tick frames (alpha = i / n) (default 1)
//   --shot-time <seconds>      animation time for intro / menu / win (default 1)
//   --shot-until-turn <head|tail>  after the ticks, keep playing until the head (or the tail)
//                              swings around a corner: checks the turn interpolation

#include <string>
#include <vector>

namespace piton_game {

struct ShotOptions {
    std::string scene;  // empty = no screenshot mode
    std::string out = "shot.png";
    int level = 1;
    int ticks = 0;
    int frames = 1;
    double time = 1.0;
    std::string until_turn;  // "", "head" or "tail"
    bool active() const { return !scene.empty(); }
};

struct LaunchOptions {
    std::string test_level;  // empty = normal start
    bool smoke = false;
    bool window_mode_forced = false;  // --windowed / --fullscreen given: ignore the saved setting
    ShotOptions shot;
};

// Parses the arguments left over by vs::parse_common_args. Unknown ones are ignored.
LaunchOptions parse_launch_options(const std::vector<std::string>& rest);

// The process-wide options (set once in main).
LaunchOptions& launch_options();

}  // namespace piton_game
