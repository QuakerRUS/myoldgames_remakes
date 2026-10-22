#include "levels.hpp"

#include <cstdio>

#include <SDL3/SDL_log.h>
#include <vs/engine/storage.hpp>

namespace sg {

namespace {
std::string two_digits(int n) {
    char buf[16];
    std::snprintf(buf, sizeof buf, "%02d", n);
    return buf;
}
}  // namespace

soldier::Level load_level(int number) {
    const std::string file = "levels/" + two_digits(number) + ".json";
    soldier::Level lv = soldier::load_level_file(vs::path_to_utf8(vs::asset_path(file)));
    if (lv.number != number) {
        // The file name decides: progress, records and the banner use the requested number.
        SDL_Log("%s says \"number\": %d; using %d (the file name)", file.c_str(), lv.number, number);
        lv.number = number;
    }
    return lv;
}

std::optional<soldier::Recording> builtin_record(int number) {
    const auto p = vs::find_asset("records/LISIY/" + two_digits(number) + ".json");
    if (p.empty()) return std::nullopt;
    try {
        return soldier::load_recording_file(vs::path_to_utf8(p));
    } catch (const std::exception& e) {
        SDL_Log("record %d: %s", number, e.what());
        return std::nullopt;
    }
}

}  // namespace sg
