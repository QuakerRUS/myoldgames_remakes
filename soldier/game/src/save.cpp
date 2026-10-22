#include "save.hpp"

#include <algorithm>
#include <cstdio>
#include <ctime>
#include <map>
#include <system_error>

#include <SDL3/SDL_log.h>
#include <nlohmann/json.hpp>
#include <soldier/progress.hpp>
#include <vs/engine/json_file.hpp>
#include <vs/engine/storage.hpp>

namespace sg {

namespace fs = std::filesystem;
using nlohmann::json;

double Settings::tick_seconds() const {
    const int i = std::clamp(speed, 0, static_cast<int>(kSpeedSteps.size()) - 1);
    return kBaseTickSeconds / kSpeedSteps[static_cast<std::size_t>(i)];
}

std::string Settings::speed_label() const {
    static const char* labels[] = {"x0.5", "x0.75", "x1", "x1.5", "x2"};
    return labels[std::clamp(speed, 0, 4)];
}

int recording_ticks(const soldier::Recording& rec) {
    if (rec.result) return rec.result->tick;
    return rec.commands.empty() ? 0 : rec.commands.back().tick;
}

SaveSystem::SaveSystem(fs::path dir) : dir_(std::move(dir)) {
    if (dir_.empty()) dir_ = vs::user_data_dir("Soldier");
    std::error_code ec;
    fs::create_directories(dir_, ec);
}

fs::path SaveSystem::slot_dir(int slot) const { return dir_ / "records" / ("slot" + std::to_string(slot + 1)); }

fs::path SaveSystem::record_path(int slot, int level) const {
    char name[16];
    std::snprintf(name, sizeof name, "%02d.json", level);
    return slot_dir(slot) / name;
}

bool SaveSystem::has_record(int slot, int level) const {
    std::error_code ec;
    return fs::is_regular_file(record_path(slot, level), ec);
}

std::optional<soldier::Recording> SaveSystem::load_record(int slot, int level) const {
    if (!has_record(slot, level)) return std::nullopt;
    try {
        return soldier::load_recording_file(vs::path_to_utf8(record_path(slot, level)));
    } catch (const std::exception& e) {
        SDL_Log("bad record %s: %s", vs::path_to_utf8(record_path(slot, level)).c_str(), e.what());
        return std::nullopt;
    }
}

bool SaveSystem::offer_record(int slot, int level, const soldier::Recording& rec) {
    if (const auto old = load_record(slot, level)) {
        if (old->result && old->result->status == soldier::Status::Won && recording_ticks(*old) <= recording_ticks(rec))
            return false;
    }
    vs::write_file_atomic(record_path(slot, level), soldier::recording_to_json(rec));
    return true;
}

void SaveSystem::create_player(int slot, const std::string& name) {
    delete_player(slot);
    PlayerSlot& s = slots[static_cast<std::size_t>(slot)];
    s.name = name;
    s.level_reached = 1;
    s.builtin = false;
    save();
}

void SaveSystem::delete_player(int slot) {
    slots[static_cast<std::size_t>(slot)] = PlayerSlot{};
    std::error_code ec;
    fs::remove_all(slot_dir(slot), ec);
    save();
}

void SaveSystem::level_completed(int slot, int level) {
    PlayerSlot& s = slots[static_cast<std::size_t>(slot)];
    s.level_reached = std::max(s.level_reached, level + 1);
    save();
}

namespace {

// Type-safe field access: a missing field or one of the wrong type gives the default, so a
// hand-edited or partly damaged save never throws.
int get_int(const json& o, const char* key, int def) {
    if (!o.is_object()) return def;
    const auto it = o.find(key);
    if (it == o.end() || !it->is_number_integer()) return def;
    const long long v = it->get<long long>();
    return static_cast<int>(std::clamp<long long>(v, -1000000, 1000000));
}

bool get_bool(const json& o, const char* key, bool def) {
    if (!o.is_object()) return def;
    const auto it = o.find(key);
    return it != o.end() && it->is_boolean() ? it->get<bool>() : def;
}

std::string get_string(const json& o, const char* key) {
    if (!o.is_object()) return {};
    const auto it = o.find(key);
    return it != o.end() && it->is_string() ? it->get<std::string>() : std::string();
}

const json& get_field(const json& o, const char* key) {
    static const json null_value;
    if (!o.is_object()) return null_value;
    const auto it = o.find(key);
    return it == o.end() ? null_value : *it;
}

// A player name as the game allows it: A-Z, 0-9, at most 8 characters.
std::string clean_name(std::string name) {
    name.erase(std::remove_if(name.begin(), name.end(),
                              [](char c) { return !((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')); }),
               name.end());
    if (name.size() > static_cast<std::size_t>(kNameLength)) name.resize(static_cast<std::size_t>(kNameLength));
    return name;
}

bool is_valid_recording(const fs::path& p) {
    try {
        (void)soldier::load_recording_file(vs::path_to_utf8(p));
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

std::string timestamp() {
    const std::time_t now = std::time(nullptr);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &now);
#else
    localtime_r(&now, &tm);
#endif
    char buf[32];
    std::strftime(buf, sizeof buf, "%Y%m%d-%H%M%S", &tm);
    return buf;
}

// ---- progress verification ----------------------------------------------------------------
// Results are cached in memory for the whole run (save.json is loaded twice at start-up): keyed
// by slot folder + player name, valid while the record files' sizes and times are unchanged.

std::string records_signature(const fs::path& dir) {
    std::string sig;
    std::error_code ec;
    for (int n = 1; n <= kLevelCount; ++n) {
        char name[16];
        std::snprintf(name, sizeof name, "%02d.json", n);
        const fs::path p = dir / name;
        const auto size = fs::file_size(p, ec);
        if (ec) break;  // verification stops at the first missing record anyway
        const auto time = fs::last_write_time(p, ec).time_since_epoch().count();
        sig += std::to_string(n) + ":" + std::to_string(size) + ":" + std::to_string(time) + ";";
    }
    return sig;
}

std::optional<soldier::Level> shipped_level(int n) {
    static std::map<int, std::optional<soldier::Level>> cache;
    if (const auto it = cache.find(n); it != cache.end()) return it->second;
    std::optional<soldier::Level> lv;
    char name[32];
    std::snprintf(name, sizeof name, "levels/%02d.json", n);
    if (const fs::path p = vs::find_asset(name); !p.empty()) {
        try {
            lv = soldier::parse_level_json(vs::read_text_file(p));
            lv->number = n;  // the file name decides (see load_level)
        } catch (const std::exception& e) {
            SDL_Log("%s: %s", name, e.what());
        }
    }
    cache[n] = lv;
    return lv;
}

int cached_verified_level(const fs::path& dir, const std::string& player) {
    struct Entry {
        std::string signature;
        int level = 1;
    };
    static std::map<std::string, Entry> cache;
    const std::string key = vs::path_to_utf8(dir) + "|" + player;
    const std::string sig = records_signature(dir);
    if (const auto it = cache.find(key); it != cache.end() && it->second.signature == sig) return it->second.level;
    const int level = soldier::verified_level_reached(
        kLevelCount, player, shipped_level, [&](int n) -> std::optional<soldier::Recording> {
            char name[16];
            std::snprintf(name, sizeof name, "%02d.json", n);
            const fs::path p = dir / name;
            std::error_code ec;
            if (!fs::is_regular_file(p, ec)) return std::nullopt;
            try {
                return soldier::parse_recording_json(vs::read_text_file(p));
            } catch (const std::exception&) {
                return std::nullopt;
            }
        });
    cache[key] = {sig, level};
    return level;
}

}  // namespace

int SaveSystem::verified_level(int slot) const {
    const PlayerSlot& s = slots[static_cast<std::size_t>(slot)];
    if (s.empty()) return 0;
    return cached_verified_level(slot_dir(slot), s.name);
}

void SaveSystem::verify_progress() {
    bool changed = false;
    for (int i = 0; i < kSlotCount; ++i) {
        PlayerSlot& s = slots[static_cast<std::size_t>(i)];
        if (s.empty()) continue;
        const int v = verified_level(i);
        if (v == s.level_reached) continue;
        SDL_Log("slot %d (%s): level_reached %d in save.json, the records prove %d; using %d", i + 1, s.name.c_str(),
                s.level_reached, v, v);
        s.level_reached = v;
        changed = true;
    }
    if (changed) save();
}

void SaveSystem::copy_builtin_records(int slot, bool fresh) const {
    const PlayerSlot& s = slots[static_cast<std::size_t>(slot)];
    std::error_code ec;
    // A slot seeded from scratch must not inherit the records of a previous player there.
    if (fresh) fs::remove_all(slot_dir(slot), ec);
    const fs::path src = vs::find_asset("records/" + s.name);
    if (src.empty() || !fs::is_directory(src, ec)) return;
    fs::create_directories(slot_dir(slot), ec);
    for (const auto& entry : fs::directory_iterator(src, ec)) {
        if (entry.path().extension() != ".json") continue;
        const fs::path dst = slot_dir(slot) / entry.path().filename();
        // Never overwrite a record the player has improved, but replace a damaged one.
        if (fs::exists(dst, ec) && is_valid_recording(dst)) continue;
        std::error_code cec;
        fs::copy_file(entry.path(), dst, fs::copy_options::overwrite_existing, cec);
        if (cec) SDL_Log("cannot restore %s: %s", vs::path_to_utf8(dst).c_str(), cec.message().c_str());
    }
}

void SaveSystem::seed_builtin() {
    const fs::path players = vs::find_asset("players.json");
    if (players.empty()) return;
    try {
        const json j = vs::read_json_file(players).value_or(json::object());
        const json& list = get_field(j, "players");
        if (!list.is_array()) return;
        int slot = 0;
        for (const json& p : list) {
            if (slot >= kSlotCount) break;
            const std::string name = clean_name(get_string(p, "name"));
            if (name.empty()) continue;
            PlayerSlot& s = slots[static_cast<std::size_t>(slot)];
            s.name = name;
            s.level_reached = std::clamp(get_int(p, "level_reached", 1), 1, kLevelCount + 1);
            s.builtin = get_bool(p, "builtin", false);
            copy_builtin_records(slot, true);
            ++slot;
        }
    } catch (const std::exception& e) {
        SDL_Log("players.json: %s", e.what());
    }
}

void SaveSystem::backup_damaged() const {
    const fs::path file = dir_ / "save.json";
    fs::path backup = dir_ / "save.json.bad";
    std::error_code ec;
    // Keep the earlier backup: a second damaged save gets a timestamped name.
    if (fs::exists(backup, ec)) backup = dir_ / ("save-" + timestamp() + ".json.bad");
    fs::rename(file, backup, ec);
    if (ec)
        SDL_Log("cannot back up the damaged save.json: %s", ec.message().c_str());
    else
        SDL_Log("the damaged save.json was kept as %s", vs::path_to_utf8(backup.filename()).c_str());
}

void SaveSystem::load() {
    std::optional<json> j;
    bool damaged = false;
    try {
        j = vs::read_json_file(dir_ / "save.json");
        if (j && !j->is_object()) {
            SDL_Log("save.json is not a JSON object; starting fresh");
            damaged = true;
        }
    } catch (const std::exception& e) {
        SDL_Log("save.json is damaged (%s); starting fresh", e.what());
        damaged = true;
    }
    if (damaged) backup_damaged();
    if (damaged || !j) {
        settings = Settings{};
        slots = {};
        selected = 0;
        seed_builtin();
        verify_progress();
        save();
        return;
    }
    try {
        const json& st = get_field(*j, "settings");
        settings.speed = std::clamp(get_int(st, "game_speed", kDefaultSpeed), 0, static_cast<int>(kSpeedSteps.size()) - 1);
        settings.sound = std::clamp(get_int(st, "sound", 8), 0, 10);
        settings.music = std::clamp(get_int(st, "music", 6), 0, 10);
        settings.fullscreen = get_bool(st, "fullscreen", false);
        settings.zoom2 = get_bool(st, "zoom2", true);
        selected = std::clamp(get_int(*j, "selected", 0), 0, kSlotCount - 1);
        const json& arr = get_field(*j, "slots");
        if (arr.is_array()) {
            for (std::size_t i = 0; i < arr.size() && i < slots.size(); ++i) {
                const json& s = arr[i];
                if (!s.is_object()) continue;
                const std::string name = clean_name(get_string(s, "name"));
                slots[i].name = name;
                slots[i].level_reached = name.empty() ? 0 : std::clamp(get_int(s, "level_reached", 1), 1, kLevelCount + 1);
                slots[i].builtin = !name.empty() && get_bool(s, "builtin", false);
                // Self-heal: a built-in player whose shipped records went missing or were
                // damaged gets them back.
                if (slots[i].builtin) copy_builtin_records(static_cast<int>(i), false);
            }
        }
    } catch (const std::exception& e) {
        // Not expected (every access above is type-checked), but a bad save must never stop the game.
        SDL_Log("save.json: %s; some settings were reset", e.what());
    }
    verify_progress();
}

void SaveSystem::save() const {
    json j;
    j["format"] = "soldier-save";
    j["version"] = 1;
    j["settings"] = {{"game_speed", settings.speed},
                     {"sound", settings.sound},
                     {"music", settings.music},
                     {"fullscreen", settings.fullscreen},
                     {"zoom2", settings.zoom2}};
    j["selected"] = selected;
    json arr = json::array();
    for (const PlayerSlot& s : slots) {
        if (s.empty())
            arr.push_back(nullptr);
        else
            arr.push_back({{"name", s.name}, {"level_reached", s.level_reached}, {"builtin", s.builtin}});
    }
    j["slots"] = arr;
    try {
        vs::write_json_file(dir_ / "save.json", j);
    } catch (const std::exception& e) {
        SDL_Log("cannot write save.json: %s", e.what());
    }
}

}  // namespace sg
