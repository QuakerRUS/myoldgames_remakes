// Settings, the 10 player slots and the players' best records, stored as JSON in the user
// data folder (%APPDATA%\V Software\Soldier\, or the engine's --data-dir):
//   save.json                      settings + slots
//   records/slot<N>/<LL>.json      best run of slot N (1-based) on level LL (recording format)
#pragma once

#include <array>
#include <filesystem>
#include <optional>
#include <string>

#include <soldier/recording.hpp>

namespace sg {

inline constexpr int kSlotCount = 10;
inline constexpr int kLevelCount = 51;
inline constexpr int kNameLength = 8;

// GAME SPEED steps; x1 (index 2) = 250 ms per tick, the pace of the original; x4 = 62.5 ms.
// save.json stores the multiplier ("game_speed_x"), not the index, so the steps may change.
inline constexpr std::array<double, 7> kSpeedSteps{0.5, 0.75, 1.0, 1.5, 2.0, 3.0, 4.0};
inline constexpr int kDefaultSpeed = 2;
inline constexpr double kBaseTickSeconds = 0.25;

// The index of the step nearest to the multiplier `x`.
int nearest_speed_step(double x);

struct Settings {
    int speed = kDefaultSpeed;  // index into kSpeedSteps
    int sound = 8;              // 0..10
    int music = 6;              // 0..10
    bool fullscreen = false;
    bool zoom2 = true;          // map drawn at 2x (64 px cells); false = 1x

    double tick_seconds() const;
    std::string speed_label() const;  // "x1", "x0.75", ..., "x4"
};

struct PlayerSlot {
    std::string name;          // empty = free slot
    int level_reached = 0;     // highest level the player may start (52 = all completed)
    bool builtin = false;      // the shipped LISIY profile
    bool empty() const { return name.empty(); }
};

class SaveSystem {
public:
    // dir empty = vs::user_data_dir("Soldier").
    explicit SaveSystem(std::filesystem::path dir = {});

    // Reads save.json; on the first launch seeds slot 1 with the built-in players from
    // data/players.json and copies their records from data/records/<NAME>/. Never throws:
    // fields of the wrong type get their defaults; a file that is not valid JSON is kept as
    // save.json.bad and the save starts fresh. level_reached is then replaced by the progress
    // the records prove (verify_progress).
    void load();
    void save() const;

    Settings settings;
    std::array<PlayerSlot, kSlotCount> slots;
    int selected = 0;  // last selected slot

    const std::filesystem::path& dir() const { return dir_; }
    std::filesystem::path record_path(int slot, int level) const;
    bool has_record(int slot, int level) const;
    std::optional<soldier::Recording> load_record(int slot, int level) const;
    // Stores `rec` if the slot has no record of that level yet or `rec` is shorter (fewer
    // ticks). Returns true if it was stored.
    bool offer_record(int slot, int level, const soldier::Recording& rec);

    void create_player(int slot, const std::string& name);
    void delete_player(int slot);
    // level_reached = max(old, completed + 1).
    void level_completed(int slot, int level);

    // Progress proven by the slot's records: 1 + the largest k such that records 01..k exist,
    // belong to this player and replay to a win at their recorded tick (soldier/progress.hpp).
    // Cached in memory per run. 0 for an empty slot.
    int verified_level(int slot) const;

private:
    std::filesystem::path dir_;
    std::filesystem::path slot_dir(int slot) const;
    void seed_builtin();
    // Copies data/records/<NAME>/ into the slot: `fresh` clears the slot's folder first;
    // otherwise only missing records and ones that fail to load are (re)placed.
    void copy_builtin_records(int slot, bool fresh) const;
    // Renames a damaged save.json to save.json.bad (or save-<time>.json.bad if that exists).
    void backup_damaged() const;
    // Sets every player's level_reached to verified_level() (logged; save.json is rewritten if
    // anything changed): editing save.json cannot unlock levels.
    void verify_progress();
};

// Ticks of a recording's game (its result tick, or the last command's tick).
int recording_ticks(const soldier::Recording& rec);

}  // namespace sg
