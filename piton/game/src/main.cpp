// Piton 2.0 — entry point: window, command line, first scene.
//
//   piton.exe                          intro, then the menu
//   piton.exe --test-level <file>      play one level (map editor F5), exit afterwards
//   piton.exe --smoke                  run one second and exit (automated checks)
//   piton.exe --shot-scene ...         debug screenshots, see launch_options.hpp
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <exception>

#include <vs/engine/app.hpp>
#include <vs/engine/storage.hpp>
#include <vs/engine/window_icon.hpp>

#include "launch_options.hpp"
#include "save_data.hpp"
#include "scenes.hpp"

namespace {

using namespace piton_game;

// Errors go to a message box, except in automated runs (no windows may pop up there).
void report_error(const char* message, SDL_Window* window) {
    const LaunchOptions& o = launch_options();
    if (o.smoke || o.shot.active())
        SDL_Log("Piton: %s", message);
    else
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Piton", message, window);
}

// The first scene for the screenshot mode.
std::unique_ptr<vs::Scene> shot_scene(const ShotOptions& s) {
    if (s.scene == "intro") return make_intro_scene(nullptr);
    if (s.scene == "menu") return make_menu_scene(nullptr, MenuPage::Main);
    if (s.scene == "options") return make_menu_scene(nullptr, MenuPage::Options);
    if (s.scene == "levels") return make_menu_scene(nullptr, MenuPage::Levels);
    if (s.scene == "win") return make_win_scene(nullptr, 123450);
    const PlayMode mode = s.scene == "demo" ? PlayMode::Demo : PlayMode::Play;
    return make_game_scene(nullptr, GameSetup{mode, s.level, 0, {}});
}

}  // namespace

int main(int argc, char** argv) {
    vs::AppConfig cfg;
    cfg.title = "Piton";
    cfg.game_id = "Piton";
    cfg.clear_color = {6, 10, 6, 255};
    const std::vector<std::string> rest = vs::parse_common_args(argc, argv, cfg);
    LaunchOptions& opts = launch_options();
    opts = parse_launch_options(rest);
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        if (a == "--smoke") opts.smoke = true;
        if (a == "--windowed" || a == "--fullscreen") opts.window_mode_forced = true;
    }
    // Automated runs and the editor's test-play never write the save, except a --smoke run
    // with an explicit --data-dir (lets the checks see a repaired, re-signed save).
    const bool smoke_into_data_dir = opts.smoke && !cfg.data_dir.empty() && !opts.shot.active() && opts.test_level.empty();
    if ((opts.shot.active() || !opts.test_level.empty() || opts.smoke) && !smoke_into_data_dir) set_saving_enabled(false);
    // Automated runs never touch %APPDATA%: --smoke does it in parse_common_args, screenshot
    // runs here (an explicit --data-dir wins).
    if (opts.shot.active() && cfg.data_dir.empty()) vs::set_user_data_smoke_mode(true);
    if (opts.shot.active()) cfg.quit_after = 0;  // the shot quits by itself

    vs::init_asset_roots(VS_DEV_DATA_DIR);
    // Fullscreen as saved, unless the command line says otherwise (or it is an automated run).
    if (!opts.window_mode_forced && !opts.smoke && !opts.shot.active() && opts.test_level.empty())
        cfg.fullscreen = SaveData::load().fullscreen;

    vs::App app(cfg);
    if (!app.init()) {
        report_error(app.error().c_str(), nullptr);
        return 1;
    }
    vs::set_window_icon(app, vs::find_asset("gfx/icon.png"));

    std::unique_ptr<vs::Scene> first;
    try {
        if (opts.shot.active())
            first = shot_scene(opts.shot);
        else if (!opts.test_level.empty())
            first = make_game_scene(nullptr, GameSetup{PlayMode::Test, 1, 0, opts.test_level});
        else
            first = make_intro_scene(nullptr);
    } catch (const std::exception& e) {
        report_error(e.what(), app.window());
        return 1;
    }
    return app.run(std::move(first));
}
