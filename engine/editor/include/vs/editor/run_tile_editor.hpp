#pragma once
#include <filesystem>
#include <string>
#include <vector>

#include <vs/editor/tile_editor.hpp>
#include <vs/engine/app.hpp>

namespace vs {

// Complete map editor program around a plugin:
//   int main(int argc, char** argv) { SoldierEditorPlugin p; return vs::run_tile_editor(p, argc, argv); }
// Window 1440x900, resizable, window-pixel coordinates (no logical canvas), no audio.
// Arguments: the common flags (--smoke quits after 1 s) plus those of parse_tile_editor_args.
// Closing the window or Esc asks to save unsaved changes.
int run_tile_editor(TileEditorPlugin& plugin, int argc, char** argv);

struct TileEditorRunOptions {
    std::filesystem::path open;  // document to open at start (optional)
    bool layout_ini = true;      // keep ImGui's layout in user_data_dir(game_id)/editor_layout.ini
};

// The editor's App settings: title and game id from the plugin, 1440x900, no logical canvas,
// no audio, dark clear colour. Adjust (e.g. fullscreen_keys) before passing it on.
AppConfig tile_editor_app_config(const TileEditorPlugin& plugin);

// Editor arguments left over by parse_common_args:
//   <file> | --open <file>   open a document at start
//   --screenshot <png>       render, save frame `screenshot_frames` as PNG, quit (sets the
//                            AppConfig screenshot fields)
// Other arguments are returned in order.
std::vector<std::string> parse_tile_editor_args(const std::vector<std::string>& args, AppConfig& config,
                                                TileEditorRunOptions& options, int screenshot_frames = 12);

// Runs the editor with a prepared configuration. Returns the process exit code.
int run_tile_editor(TileEditorPlugin& plugin, const AppConfig& config, const TileEditorRunOptions& options);

}  // namespace vs
