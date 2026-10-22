#pragma once
// Runtime window icon (title bar / taskbar icon of the SDL window). The .exe icon itself is a
// Windows resource: see "Application icons" in engine/README.md.
#include <filesystem>
#include <vector>

#include <vs/engine/image.hpp>

struct SDL_Window;

namespace vs {

class App;

// Sets `icon` as the window icon; `alternates` (other sizes, e.g. hand-tuned 16/32/48 px
// versions) let the OS pick the best match. Returns false on failure; never throws.
bool set_window_icon(SDL_Window* window, const Image& icon, const std::vector<Image>& alternates = {});
// Loads PNG files (missing or broken alternates and empty paths are skipped). Returns false if
// the main icon cannot be loaded or set; never throws.
//   vs::set_window_icon(app, vs::find_asset("gfx/icon.png"), {vs::find_asset("gfx/icon_16.png")});
bool set_window_icon(SDL_Window* window, const std::filesystem::path& png,
                     const std::vector<std::filesystem::path>& alternates = {});
bool set_window_icon(App& app, const std::filesystem::path& png, const std::vector<std::filesystem::path>& alternates = {});

}  // namespace vs
