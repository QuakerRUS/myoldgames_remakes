// Painter — the shared graphics editor of the remakes (PNG images and sprite sheets).
//
//   painter [file.png | sheet.json]          open a file
//   painter --open <file> [--frame <name>]   same; --frame shows that frame alone
//   painter --screenshot <out.png> ...       render a few frames, save one, quit (visual checks)
//   painter --roundtrip <dir> <outdir>       open+save every sheet/PNG of <dir> into <outdir>, compare
//   painter --selftest <scratch dir> --open <sheet.json>   scripted edits + undo/redo/save checks
//   painter --smoke                          start, run one second, quit
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <iostream>

#include <vs/engine/app.hpp>
#include <vs/engine/console.hpp>
#include <vs/engine/json_file.hpp>

#include "painter_scene.hpp"
#include "roundtrip.hpp"

int main(int argc, char** argv) {
    vs::AppConfig cfg;
    cfg.title = "Painter \xE2\x80\x94 V Software";
    cfg.game_id = "Painter";
    cfg.window_width = 1440;
    cfg.window_height = 900;
    cfg.logical_width = 0;
    cfg.logical_height = 0;
    cfg.enable_audio = false;
    cfg.fullscreen_keys = false;  // F11 / Alt+Enter are not needed in an editor
    cfg.clear_color = {30, 32, 37, 255};
    const std::vector<std::string> rest = vs::parse_common_args(argc, argv, cfg);

    painter::LaunchOptions options;
    for (std::size_t i = 0; i < rest.size(); ++i) {
        const std::string& a = rest[i];
        const bool has_value = i + 1 < rest.size();
        if (a == "--roundtrip") {
            vs::attach_parent_console();
            if (i + 2 >= rest.size()) {
                std::cerr << "usage: painter --roundtrip <dir> <outdir>\n";
                return 2;
            }
            const int code = painter::run_roundtrip(vs::utf8_to_path(rest[i + 1]), vs::utf8_to_path(rest[i + 2]), std::cout);
            std::cout.flush();
            return code;
        }
        if (a == "--open" && has_value)
            options.open = vs::utf8_to_path(rest[++i]);
        else if (a == "--frame" && has_value)
            options.frame = rest[++i];
        else if (a == "--selftest" && has_value)
            options.selftest = vs::utf8_to_path(rest[++i]);
        else if (a == "--screenshot" && has_value) {
            // A few frames first: the layout settles (fit to window, list scrolling) before the shot.
            cfg.screenshot_path = vs::utf8_to_path(rest[++i]);
            cfg.screenshot_after_frames = 12;
        }
        else if (!a.empty() && a[0] != '-')
            options.open = vs::utf8_to_path(a);
    }

    vs::App app(cfg);
    if (!app.init()) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, cfg.title.c_str(), app.error().c_str(), nullptr);
        return 1;
    }
    return app.run(std::make_unique<painter::PainterScene>(std::move(options)));
}
