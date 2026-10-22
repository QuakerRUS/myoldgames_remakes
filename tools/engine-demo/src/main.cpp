// engine-demo: manual and automated (--smoke) check of vs_engine / vs_editor.
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <cstring>

#include <vs/engine/app.hpp>

#include "demo.hpp"

int main(int argc, char** argv) {
    for (int i = 1; i < argc; ++i)
        if (std::strcmp(argv[i], "--editor") == 0) return demo::run_demo_editor(argc, argv);

    vs::AppConfig cfg;
    cfg.title = "Engine demo";
    cfg.game_id = "Engine Demo";
    cfg.clear_color = {8, 8, 10, 255};
    const std::vector<std::string> rest = vs::parse_common_args(argc, argv, cfg);
    for (std::size_t i = 0; i + 1 < rest.size(); ++i)
        if (rest[i] == "--screenshot") demo::screenshot_path() = rest[i + 1];
    vs::App app(cfg);
    if (!app.init()) {
        SDL_Log("init failed: %s", app.error().c_str());
        return 1;
    }
    return app.run(demo::make_demo_scene());
}
