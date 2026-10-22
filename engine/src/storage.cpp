#include <vs/engine/storage.hpp>

#include <system_error>

#include <SDL3/SDL.h>

#include <vs/engine/error.hpp>

namespace vs {

namespace fs = std::filesystem;

namespace {
std::vector<fs::path>& roots_storage() {
    static std::vector<fs::path> roots;
    return roots;
}
fs::path& data_dir_override_storage() {
    static fs::path dir;
    return dir;
}
bool& smoke_mode_storage() {
    static bool on = false;
    return on;
}
fs::path ensure_dir(const fs::path& dir) {
    std::error_code ec;
    fs::create_directories(dir, ec);
    if (ec && !fs::is_directory(dir))
        throw Error("cannot create the user data folder " + path_to_utf8(dir) + ": " + ec.message());
    return dir;
}
}  // namespace

void set_user_data_dir_override(const fs::path& dir) { data_dir_override_storage() = dir; }
const fs::path& user_data_dir_override() { return data_dir_override_storage(); }
void set_user_data_smoke_mode(bool on) { smoke_mode_storage() = on; }
bool user_data_smoke_mode() { return smoke_mode_storage(); }

fs::path user_data_dir(std::string_view game) {
    if (!data_dir_override_storage().empty()) return ensure_dir(data_dir_override_storage());
    if (smoke_mode_storage()) {
        std::error_code ec;
        fs::path tmp = fs::temp_directory_path(ec);
        if (ec) throw Error("cannot get the temporary folder: " + ec.message());
        return ensure_dir(tmp / "vs-smoke" / utf8_to_path(game));
    }
    const std::string name(game);
    char* p = SDL_GetPrefPath("V Software", name.c_str());
    if (!p) throw Error(std::string("cannot get the user data folder: ") + SDL_GetError());
    fs::path dir = utf8_to_path(p);
    SDL_free(p);
    std::error_code ec;
    fs::create_directories(dir, ec);
    return dir;
}

fs::path exe_dir() {
    const char* base = SDL_GetBasePath();  // owned by SDL
    return base ? utf8_to_path(base) : fs::current_path();
}

void init_asset_roots(std::string_view dev_data_dir) {
    // <exe_dir>/data first: the build copies each program's data next to it (every program has
    // its own output folder), and a shipped exe has only that. The source tree's data folder is
    // a development fallback (e.g. a file added to the source data but not yet copied).
    std::vector<fs::path> roots{exe_dir() / "data"};
    std::error_code ec;
    const fs::path dev = dev_data_dir.empty() ? fs::path() : utf8_to_path(dev_data_dir);
    if (!dev.empty() && fs::is_directory(dev, ec) && !fs::equivalent(dev, roots.front(), ec)) roots.push_back(dev);
    set_asset_roots(std::move(roots));
}

void set_asset_roots(std::vector<fs::path> roots) { roots_storage() = std::move(roots); }

const std::vector<fs::path>& asset_roots() { return roots_storage(); }

fs::path find_asset(std::string_view relative) {
    const fs::path rel = utf8_to_path(relative);
    std::error_code ec;
    for (const fs::path& root : roots_storage()) {
        fs::path p = root / rel;
        if (fs::exists(p, ec)) return p.lexically_normal();
    }
    return {};
}

fs::path asset_path(std::string_view relative) {
    fs::path p = find_asset(relative);
    if (!p.empty()) return p;
    std::string msg = "asset not found: " + std::string(relative) + " (searched:";
    for (const fs::path& root : roots_storage()) msg += " " + path_to_utf8(root);
    throw Error(msg + ")");
}

}  // namespace vs
