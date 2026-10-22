#include <vs/editor/run_tile_editor.hpp>

#include <memory>
#include <optional>
#include <stdexcept>

#include <SDL3/SDL.h>

#include <vs/engine/json_file.hpp>
#include <vs/engine/storage.hpp>
#include <vs/imgui/imgui_layer.hpp>

namespace vs {

namespace {

class TileEditorScene : public Scene {
public:
    TileEditorScene(TileEditorPlugin& plugin, TileEditorRunOptions options) : plugin_(plugin), options_(std::move(options)) {}

    void on_enter() override {
        if (editor_) return;
        ImGuiLayerOptions opts;
        if (options_.layout_ini) opts.ini_path = user_data_dir(plugin_.game_id()) / "editor_layout.ini";
        if (!layer_.attach(app(), opts)) throw std::runtime_error("ImGui initialization failed");
        plugin_.load_resources(app().renderer());
        editor_ = std::make_unique<TileEditor>(plugin_, app().window());
        if (!options_.open.empty()) editor_->open(options_.open);
    }
    void update(double) override {
        if (editor_ && editor_->quit_requested()) app().quit();
    }
    void render(Renderer&, double) override {
        layer_.new_frame();
        editor_->draw();
        layer_.render();
    }
    bool on_quit_request() override { return !editor_ || editor_->request_quit(); }

private:
    TileEditorPlugin& plugin_;
    TileEditorRunOptions options_;
    ImGuiLayer layer_;
    std::unique_ptr<TileEditor> editor_;  // declared after layer_: destroyed first
};

}  // namespace

AppConfig tile_editor_app_config(const TileEditorPlugin& plugin) {
    AppConfig cfg;
    cfg.title = plugin.title();
    cfg.game_id = plugin.game_id();
    cfg.window_width = 1440;
    cfg.window_height = 900;
    cfg.logical_width = 0;
    cfg.logical_height = 0;
    cfg.enable_audio = false;
    cfg.clear_color = {24, 26, 30, 255};
    return cfg;
}

std::vector<std::string> parse_tile_editor_args(const std::vector<std::string>& args, AppConfig& config,
                                                TileEditorRunOptions& options, int screenshot_frames) {
    std::vector<std::string> rest;
    for (std::size_t i = 0; i < args.size(); ++i) {
        const std::string& a = args[i];
        const bool has_value = i + 1 < args.size();
        if (a == "--screenshot" && has_value) {
            config.screenshot_path = utf8_to_path(args[++i]);
            config.screenshot_after_frames = screenshot_frames;  // the field is fitted once its size is known
        } else if (a == "--open" && has_value) {
            options.open = utf8_to_path(args[++i]);
        } else if (!a.empty() && a[0] != '-' && options.open.empty()) {
            options.open = utf8_to_path(a);
        } else {
            rest.push_back(a);
        }
    }
    return rest;
}

int run_tile_editor(TileEditorPlugin& plugin, const AppConfig& config, const TileEditorRunOptions& options) {
    App app(config);
    if (!app.init()) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, config.title.c_str(), app.error().c_str(), nullptr);
        return 1;
    }
    return app.run(std::make_unique<TileEditorScene>(plugin, options));
}

int run_tile_editor(TileEditorPlugin& plugin, int argc, char** argv) {
    AppConfig cfg = tile_editor_app_config(plugin);
    TileEditorRunOptions options;
    parse_tile_editor_args(parse_common_args(argc, argv, cfg), cfg, options);
    return run_tile_editor(plugin, cfg, options);
}

}  // namespace vs
