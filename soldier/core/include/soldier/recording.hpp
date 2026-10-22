// Recordings (demos): the list of executed commands with their ticks, and their playback.
#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "soldier/game.hpp"

namespace soldier {

struct RecordedResult {
    Status status = Status::Won;
    int tick = 0;  // tick at which the game ended
    std::optional<DeathCause> cause;  // when status == Dead
    friend bool operator==(const RecordedResult&, const RecordedResult&) = default;
};

struct Recording {
    int level = 0;
    std::string player;
    std::vector<RecordedCommand> commands;  // ticks are non-decreasing
    std::optional<RecordedResult> result;   // how the recorded game ended, if it did
    std::string source;                     // informational (e.g. "REC023.DAT"); may be empty
};

// JSON (docs/formats/soldier-recording.md). parse/load throw FormatError.
Recording parse_recording_json(std::string_view text);
Recording load_recording_file(const std::string& path);
std::string recording_to_json(const Recording& r);

// Builds a recording from a finished (or abandoned) game.
Recording make_recording(const Game& game, int level_number, std::string player);

// Step-by-step playback for the presentation layer:
//     Replayer r(level, rec);            // applies the commands of tick 0
//     every tick period: r.step();       // game().tick(), then the commands of the new tick
// Draw r.game() and consume r.game().take_events() as in live play.
class Replayer {
public:
    Replayer(Level level, Recording recording);
    Game& game() { return game_; }
    const Game& game() const { return game_; }
    void step();
    bool commands_left() const { return next_ < rec_.commands.size(); }
    bool finished() const { return game_.status() != Status::Playing; }
    std::size_t applied() const { return next_; }  // commands handed to the game so far

private:
    Game game_;
    Recording rec_;
    std::size_t next_ = 0;
    void apply_due();
};

struct ReplayOutcome {
    Status status = Status::Playing;
    DeathCause cause = DeathCause::GaveUp;
    int tick = 0;
    std::size_t unused_commands = 0;  // commands not applied because the game ended first
    // true if every applied command was executed at its recorded tick (none was buffered by
    // the move cooldown), i.e. the game's history() equals the recording.
    bool in_sync = false;
};

// Plays a recording to the end, ticking at most `extra_ticks` past the last command.
ReplayOutcome run_replay(const Level& level, const Recording& rec, int extra_ticks = 4000);

}  // namespace soldier
