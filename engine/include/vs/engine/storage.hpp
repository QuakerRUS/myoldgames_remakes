#pragma once
#include <filesystem>
#include <string_view>
#include <vector>

#include <vs/engine/json_file.hpp>

namespace vs {

// Per-user data folder `%APPDATA%\V Software\<game>\` (SDL_GetPrefPath("V Software", game)).
// Created if missing. Throws vs::Error if SDL cannot provide it (or the folder cannot be created).
//   - set_user_data_dir_override(path) set (non-empty; `--data-dir <path>` via parse_common_args):
//     returns that path for every game (created if needed); %APPDATA% is never touched.
//   - else in smoke mode (set_user_data_smoke_mode(true); `--smoke` via parse_common_args):
//     returns temp_directory_path()/vs-smoke/<game> (created), so smoke tests never write into
//     the real profile.
std::filesystem::path user_data_dir(std::string_view game);
// Empty path clears the override.
void set_user_data_dir_override(const std::filesystem::path& dir);
const std::filesystem::path& user_data_dir_override();
void set_user_data_smoke_mode(bool on);
bool user_data_smoke_mode();

// Folder of the running executable (SDL_GetBasePath()).
std::filesystem::path exe_dir();

// Asset lookup. Roots are searched in order; the defaults set by init_asset_roots are
//   1. <exe_dir>/data            (copied there by vs_add_game_data at build time; shipped exes)
//   2. dev_data_dir              (the source tree's data folder, VS_DEV_DATA_DIR) if it exists and
//                                differs: a development fallback for files missing in the copy
// Every program has its own output folder (bin/<Config>/<Program>/), so the copies of different
// programs never overwrite each other.
void init_asset_roots(std::string_view dev_data_dir = {});
void set_asset_roots(std::vector<std::filesystem::path> roots);
const std::vector<std::filesystem::path>& asset_roots();
// Full path of `relative` (forward slashes, UTF-8, e.g. "sprites/hero.json") under the first
// root where it exists. Throws vs::Error listing the searched roots if it is found nowhere.
std::filesystem::path asset_path(std::string_view relative);
// Like asset_path, but returns an empty path instead of throwing.
std::filesystem::path find_asset(std::string_view relative);

}  // namespace vs
