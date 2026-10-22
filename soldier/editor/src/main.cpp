// Soldier 2.0 Map Editor: vs::TileEditor (engine/editor) driven by SoldierPlugin.
//   soldier_editor [level.json]                 open a level (also: --open <level.json>)
//   soldier_editor --roundtrip <dir>            load + save every *.json of <dir> in memory and
//                                               compare the bytes (no window); exit code 0 = all identical
//   soldier_editor --open <lvl> --screenshot <png>   render the editor (12 frames) into a PNG and quit
//   --smoke, --windowed, --fullscreen           common engine flags
#include <SDL3/SDL_main.h>

#include <cstdio>

#include <vs/editor/run_tile_editor.hpp>
#include <vs/engine/console.hpp>
#include <vs/engine/storage.hpp>

#include "level_io.hpp"
#include "soldier_plugin.hpp"

#ifndef VS_DEV_DATA_DIR
#define VS_DEV_DATA_DIR ""
#endif

namespace {

int run_round_trip(const std::string& dir) {
    vs::attach_parent_console();
    const soldier_editor::RoundTripResult r = soldier_editor::round_trip_directory(vs::utf8_to_path(dir));
    for (const std::string& f : r.failures) std::printf("FAIL %s\n", f.c_str());
    std::printf("round trip: %d/%d files byte-identical in %s\n", r.identical, r.files, dir.c_str());
    std::fflush(stdout);
    return r.files > 0 && r.failures.empty() ? 0 : 1;
}

}  // namespace

int main(int argc, char** argv) {
    vs::init_asset_roots(VS_DEV_DATA_DIR);
    soldier_editor::SoldierPlugin plugin{vs::utf8_to_path(VS_DEV_DATA_DIR)};
    vs::AppConfig cfg = vs::tile_editor_app_config(plugin);
    cfg.fullscreen_keys = false;  // F11 / Alt+Enter belong to nobody in the editor; keep it windowed
    const std::vector<std::string> rest = vs::parse_common_args(argc, argv, cfg);
    for (std::size_t i = 0; i + 1 < rest.size(); ++i)
        if (rest[i] == "--roundtrip") return run_round_trip(rest[i + 1]);
    vs::TileEditorRunOptions options;
    options.layout_ini = false;
    vs::parse_tile_editor_args(rest, cfg, options, 12);
    return vs::run_tile_editor(plugin, cfg, options);
}
