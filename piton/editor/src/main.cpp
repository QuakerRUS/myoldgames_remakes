// Piton Map Editor (piton_editor.exe): vs::TileEditor (engine/editor) driven by PitonPlugin.
//   piton_editor [level.json]               edit a level (default folder: piton/data/levels)
//   piton_editor --roundtrip <dir>          self-check: load+save every *.json, compare bytes
//   piton_editor <level> --screenshot out.png   render 30 frames, save the last, quit
//   --smoke                                 start, run one second, exit 0
#include <SDL3/SDL_main.h>

#include <cstdio>
#include <cstring>

#include <vs/editor/run_tile_editor.hpp>
#include <vs/engine/console.hpp>
#include <vs/engine/storage.hpp>

#include "level_doc.hpp"
#include "piton_editor.hpp"

namespace {

int runRoundTrip(const char* dir) {
    vs::attach_parent_console();
    const piton_editor::RoundTripReport r = piton_editor::roundTrip(vs::utf8_to_path(dir));
    for (const std::string& line : r.lines) std::printf("%s\n", line.c_str());
    std::printf("roundtrip: %d file(s), %d failure(s)\n", r.files, r.failures);
    std::fflush(stdout);
    return r.files > 0 && r.failures == 0 ? 0 : 1;
}

std::filesystem::path defaultLevelDir() {
    const std::filesystem::path dev = vs::utf8_to_path(VS_DEV_DATA_DIR) / "levels";
    std::error_code ec;
    if (std::filesystem::is_directory(dev, ec)) return dev;
    return vs::exe_dir() / "data" / "levels";
}

}  // namespace

int main(int argc, char** argv) {
    for (int i = 1; i < argc; ++i)
        if (std::strcmp(argv[i], "--roundtrip") == 0) {
            if (i + 1 >= argc) {
                vs::attach_parent_console();
                std::fprintf(stderr, "usage: piton_editor --roundtrip <levels dir>\n");
                return 2;
            }
            return runRoundTrip(argv[i + 1]);
        }

    vs::init_asset_roots(VS_DEV_DATA_DIR);
    piton_editor::PitonPlugin plugin(defaultLevelDir());
    vs::AppConfig cfg = vs::tile_editor_app_config(plugin);
    vs::TileEditorRunOptions options;
    vs::parse_tile_editor_args(vs::parse_common_args(argc, argv, cfg), cfg, options, 30);
    return vs::run_tile_editor(plugin, cfg, options);
}
