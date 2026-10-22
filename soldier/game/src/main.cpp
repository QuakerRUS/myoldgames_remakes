// Soldier 2.0 — entry point: window, icon, command line and the first scene.
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <cstdio>
#include <cstring>
#include <memory>

#include <vs/engine/app.hpp>
#include <vs/engine/console.hpp>
#include <vs/engine/storage.hpp>
#include <vs/engine/window_icon.hpp>

#include "cli.hpp"
#include "game_scene.hpp"
#include "levels.hpp"
#include "replay_scene.hpp"
#include "resources.hpp"
#include "verify_scene.hpp"

namespace {

bool has_arg(int argc, char** argv, const char* a) {
    for (int i = 1; i < argc; ++i)
        if (std::strcmp(argv[i], a) == 0) return true;
    return false;
}

std::unique_ptr<vs::Scene> first_scene(const sg::ResourcesPtr& res) {
    const sg::CommandLine& cl = res->cmd;
    if (cl.verify_replays) return std::make_unique<sg::VerifyScene>(res);
    if (!cl.test_level.empty()) {
        sg::GameOptions o;
        o.test_level = true;
        return std::make_unique<sg::GameScene>(res, soldier::load_level_file(cl.test_level), o);
    }
    if (cl.scene == "menu") return sg::make_menu_scene(res);
    if (cl.scene == "game") {
        sg::GameOptions o;
        o.slot = res->save.selected;
        if (res->save.slots[static_cast<std::size_t>(o.slot)].empty()) o.slot = -1;
        return std::make_unique<sg::GameScene>(res, sg::load_level(cl.level), o);
    }
    if (cl.scene == "replay") {
        auto rec = cl.record.empty() ? sg::builtin_record(cl.level)
                                     : std::optional<soldier::Recording>(soldier::load_recording_file(cl.record));
        if (!rec) throw std::runtime_error("no record for level " + std::to_string(cl.level));
        return std::make_unique<sg::ReplayScene>(res, sg::load_level(rec->level), *rec);
    }
    return sg::make_intro_scene(res);
}

}  // namespace

int main(int argc, char** argv) {
    vs::AppConfig cfg;
    cfg.title = "Soldier 2.0";
    cfg.game_id = "Soldier";
    cfg.clear_color = {0, 0, 0, 255};
    const std::vector<std::string> rest = vs::parse_common_args(argc, argv, cfg);
    sg::CommandLine cl = sg::parse_command_line(rest);
    if (cl.verify_replays || cl.log_events) vs::attach_parent_console();  // command-line output
    if (cl.verify_replays) cfg.enable_audio = false;
    vs::init_asset_roots(VS_DEV_DATA_DIR);

    // The saved fullscreen setting applies unless the command line decides.
    // (--data-dir is handled by vs::parse_common_args: SaveSystem's default folder follows it.)
    try {
        sg::SaveSystem probe;
        probe.load();
        if (!has_arg(argc, argv, "--windowed") && !has_arg(argc, argv, "--fullscreen") && cl.screenshot.empty() &&
            cfg.quit_after <= 0 && !cl.verify_replays)
            cfg.fullscreen = probe.settings.fullscreen;
    } catch (const std::exception& e) {
        SDL_Log("save: %s", e.what());  // never fatal: the game starts with the defaults
    }

    vs::App app(cfg);
    if (!app.init()) {
        SDL_Log("init failed: %s", app.error().c_str());
        return 1;
    }
    vs::set_window_icon(app, vs::find_asset("gfx/icon.png"));
    sg::update_canvas_scaling(app);

    std::unique_ptr<vs::Scene> first;
    {
        sg::ResourcesPtr res;
        try {
            res = std::make_shared<sg::Resources>(app, std::move(cl));
            // Every fullscreen change (OPTIONS, F11, Alt+Enter, the system) is saved.
            app.set_fullscreen_listener([weak = std::weak_ptr<sg::Resources>(res)](bool on) {
                if (const auto r = weak.lock()) {
                    r->save.settings.fullscreen = on;
                    r->save.save();
                }
            });
            first = first_scene(res);
        } catch (const std::exception& e) {
            SDL_Log("start-up failed: %s", e.what());
            std::fprintf(stderr, "start-up failed: %s\n", e.what());
            if (cfg.quit_after <= 0 && res && res->cmd.screenshot.empty())
                SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Soldier 2.0", e.what(), app.window());
            return 1;
        }
    }
    return app.run(std::move(first));
}
