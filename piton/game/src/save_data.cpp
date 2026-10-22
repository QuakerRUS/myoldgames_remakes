#include "save_data.hpp"

#include <exception>

#include <SDL3/SDL_log.h>
#include <nlohmann/json.hpp>
#include <vs/engine/storage.hpp>

#include "save_codec.hpp"

namespace piton_game {
namespace {

bool g_saving_enabled = true;

std::filesystem::path save_file() { return vs::user_data_dir("Piton") / "piton.json"; }

}  // namespace

void set_saving_enabled(bool enabled) { g_saving_enabled = enabled; }
bool saving_enabled() { return g_saving_enabled; }

SaveData SaveData::load() {
    try {
        const std::optional<nlohmann::json> j = vs::read_json_file(save_file());
        if (!j) return SaveData{};  // first start
        SaveData s = save_from_json(*j);
        if (s.tampered) SDL_Log("Piton: the save file was modified; progress reset");
        return s;
    } catch (const std::exception& e) {
        SDL_Log("Piton: the save file is broken (%s); progress reset", e.what());
        SaveData s;
        s.tampered = true;
        return s;
    }
}

void SaveData::save() const {
    if (!g_saving_enabled) return;
    try {
        vs::write_json_file(save_file(), save_to_json(*this));
    } catch (const std::exception& e) {
        SDL_Log("Piton: cannot write the save file: %s", e.what());
    }
}

}  // namespace piton_game
