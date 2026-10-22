// Keyboard Simulator 2.0 — touch-typing trainer (remake of the 2007 thesis program).
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <memory>

#include <vs/engine/app.hpp>
#include <vs/engine/storage.hpp>
#include <vs/engine/window_icon.hpp>

#include "options.hpp"
#include "palette.hpp"
#include "trainer_scene.hpp"

int main(int argc, char** argv) {
    vs::AppConfig cfg;
    cfg.title = "Keyboard Simulator — Клавиатурный тренажёр";
    cfg.game_id = "Keyboard Simulator";
    cfg.clear_color = ks::pal::background;
    const std::vector<std::string> rest = vs::parse_common_args(argc, argv, cfg);
    ks::Options options = ks::parse_options(rest);
    // Automated runs never touch %APPDATA% (--smoke does this in parse_common_args; scripted
    // screenshot runs too); an explicit --data-dir wins.
    if (options.scripted() && cfg.data_dir.empty()) vs::set_user_data_smoke_mode(true);
    vs::init_asset_roots(VS_DEV_DATA_DIR);

    vs::App app(cfg);
    if (!app.init()) {
        SDL_Log("init failed: %s", app.error().c_str());
        return 1;
    }
    vs::set_window_icon(app, vs::find_asset("gfx/icon.png"),
                        {vs::find_asset("gfx/icon_16.png"), vs::find_asset("gfx/icon_32.png"), vs::find_asset("gfx/icon_48.png")});
    return app.run(std::make_unique<ks::TrainerScene>(std::move(options)));
}
