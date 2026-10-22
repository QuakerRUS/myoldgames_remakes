#include "settings.hpp"

#include <nlohmann/json.hpp>
#include <vs/engine/json_file.hpp>
#include <vs/engine/storage.hpp>

namespace ks {

namespace {
constexpr const char* kGameId = "Keyboard Simulator";

std::filesystem::path settings_file() { return vs::user_data_dir(kGameId) / "settings.json"; }
}  // namespace

Settings Settings::load() {
    Settings s;
    try {
        if (auto j = vs::read_json_file(settings_file()); j && j->is_object())
            s.sound = j->value("sound", s.sound);
    } catch (...) {
    }
    return s;
}

void Settings::save() const {
    try {
        vs::write_json_file(settings_file(), nlohmann::json{{"sound", sound}});
    } catch (...) {
    }
}

}  // namespace ks
