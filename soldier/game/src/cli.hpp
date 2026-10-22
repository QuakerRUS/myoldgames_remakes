// Command-line options of soldier.exe (besides the engine's --smoke, --windowed, --fullscreen and
// --data-dir <dir>, which makes vs::user_data_dir() and so the saves use <dir> instead of %APPDATA%).
#pragma once

#include <string>
#include <vector>

namespace sg {

struct CommandLine {
    // --test-level <path.json>: play that level directly (map editor F5); the program quits
    // after the level ends.
    std::string test_level;

    // --verify-replays: plays every built-in LISIY record through the replay scene at high
    // speed, prints the results and exits with 0 only if every record ends in a win.
    bool verify_replays = false;

    // ---- debug / automated visual checks ----
    std::string scene;           // --scene intro|menu|game|replay: start there
    int level = 1;               // --level N (game / replay scenes)
    std::string record;          // --record <path.json> (replay scene; default: the LISIY record)
    std::string screenshot;      // --screenshot <file.png>: save one frame and quit
    double shot_time = 1.5;      // --shot-time S: scene time (seconds) of the screenshot
    int shot_tick = -1;          // --shot-tick N: level scenes shoot once the game reaches tick N
    double shot_delay = 0.25;    // --shot-delay S: ... plus S seconds (or S after the game ended)
    double time_scale = 1.0;     // --time-scale K: run all scene clocks K times faster
    bool autoplay_record = false;  // --autoplay-record: the game scene plays the LISIY record as input
    int give_up_at = -1;         // --give-up-at N: the game scene presses Esc at tick N
    std::string menu_open;       // --menu-open newgame|level|delete|options|exit|denied|norecord|win
    double replay_speed = 1.0;   // --replay-speed 1|2|4
    bool log_events = false;     // --log-events: print every game event with its tick
    std::string script;          // --script "<t> <key>; <t> text <TEXT>; ..." (see input_script.hpp)

    std::vector<std::string> unknown;
};

// Parses the arguments left over by vs::parse_common_args.
CommandLine parse_command_line(const std::vector<std::string>& args);

}  // namespace sg
