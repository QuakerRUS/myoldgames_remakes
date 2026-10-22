#pragma once
// Player progress and settings, stored as JSON in vs::user_data_dir("Piton")/piton.json.
// The progress (level_reached, high_score) is signed, see save_codec.hpp.

namespace piton_game {

struct SaveData {
    int level_reached = 1;     // highest unlocked level, 1..10
    int high_score = 0;
    int sound_volume = 80;     // percent, 0..100 in steps of 10
    int music_volume = 70;     // percent
    bool fullscreen = false;

    // Not stored: load() found the progress edited (bad or missing "sig") or the file broken,
    // and reset level_reached / high_score (settings kept). The menu tells the player once.
    bool tampered = false;

    // Reads the save file; a missing file gives the defaults. Never throws.
    static SaveData load();
    // Writes the save file (atomic). Does nothing when saving is disabled.
    void save() const;
};

// Screenshot runs and the editor's test-play must not touch the player's save.
void set_saving_enabled(bool enabled);
bool saving_enabled();

}  // namespace piton_game
