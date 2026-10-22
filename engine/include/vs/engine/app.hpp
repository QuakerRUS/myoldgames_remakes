#pragma once
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <vs/engine/audio.hpp>
#include <vs/engine/input.hpp>
#include <vs/engine/math.hpp>
#include <vs/engine/renderer.hpp>
#include <vs/engine/scene.hpp>

struct SDL_Window;
struct SDL_Renderer;

namespace vs {

struct AppConfig {
    std::string title = "V Software";
    std::string game_id;  // folder name under %APPDATA%\V Software (see user_data_dir)
    int window_width = 1280;
    int window_height = 720;
    // Logical canvas, letterboxed into the window. 0 disables logical presentation
    // (editors: draw in window pixels).
    int logical_width = 1280;
    int logical_height = 720;
    bool integer_scale = false;  // scale the canvas only by whole multiples (sharp pixel art)
    bool fullscreen = false;     // borderless desktop fullscreen
    bool vsync = true;
    bool resizable = true;
    double logic_step = 1.0 / 60.0;  // seconds per update()
    int max_steps_per_frame = 8;     // catch-up limit after a stall
    bool fullscreen_keys = true;     // Alt+Enter and F11 toggle fullscreen
    bool enable_audio = true;
    Color clear_color{0, 0, 0, 255};  // letterbox bars and the initial clear
    double quit_after = 0.0;          // > 0: quit automatically after this many seconds (smoke tests)
    // Automated visual checks: when set, App saves the frame rendered as number
    // `screenshot_after_frames` (1 = the first; frames before it let layouts settle) as PNG,
    // then quits with 0 (1 if saving failed). For shots that depend on scene state, call
    // vs::save_frame_png (screenshot.hpp) from Scene::render instead.
    std::filesystem::path screenshot_path;
    int screenshot_after_frames = 1;
    // Set by parse_common_args: --data-dir <path> (user_data_dir() returns it) and --smoke.
    std::filesystem::path data_dir;
    bool smoke = false;
};

// Applies common command-line flags to `config`:
//   --smoke             quit_after = 1.0, smoke = true (start, run one second, exit 0); without
//                       --data-dir, user_data_dir() then uses %TEMP%/vs-smoke/<game> instead of
//                       %APPDATA% (set_user_data_smoke_mode)
//   --data-dir <path>   data_dir = path (also --data-dir=<path>); user_data_dir() returns it
//                       (set_user_data_dir_override)
//   --windowed          fullscreen = false
//   --fullscreen        fullscreen = true
// Unknown arguments are returned (in order) for the program to handle.
std::vector<std::string> parse_common_args(int argc, char** argv, AppConfig& config);

// Window + renderer + audio + input + scene stack + main loop.
//
//   int main(int argc, char** argv) {
//       vs::AppConfig cfg; cfg.title = "Soldier 2.0"; cfg.game_id = "Soldier";
//       vs::parse_common_args(argc, argv, cfg);
//       vs::App app(cfg);
//       if (!app.init()) return 1;
//       return app.run(std::make_unique<IntroScene>());
//   }
class App {
public:
    explicit App(AppConfig config);
    ~App();
    App(const App&) = delete;
    App& operator=(const App&) = delete;

    // Initializes SDL (video, audio, events), creates the window and renderer. Returns false
    // on failure (error()). Audio failing is not fatal (audio().available() is false).
    bool init();
    const std::string& error() const { return error_; }

    // Pushes `first` and runs the loop until quit() or the stack becomes empty.
    // Returns the code passed to quit() (0 by default). Exceptions escaping a scene are
    // reported in a message box and make run() return 1.
    int run(std::unique_ptr<Scene> first);
    void quit(int exit_code = 0);

    // Scene stack. Changes requested during update/event/render are applied after the
    // current call returns, so a scene may safely replace or pop itself.
    void push_scene(std::unique_ptr<Scene> scene);
    void pop_scene();
    void replace_scene(std::unique_ptr<Scene> scene);  // pop + push
    Scene* top_scene() const;
    std::size_t scene_count() const { return scenes_.size(); }

    // Fullscreen (borderless desktop). is_fullscreen() is the current (requested) state, also
    // updated when the system changes it; fullscreen() asks the window flags (kept for
    // compatibility; may lag behind a request on asynchronous platforms).
    void set_fullscreen(bool on);
    bool fullscreen() const;
    bool is_fullscreen() const { return config_.fullscreen; }
    void toggle_fullscreen() { set_fullscreen(!is_fullscreen()); }
    // Called with the new state after every fullscreen change: set_fullscreen /
    // toggle_fullscreen, Alt+Enter / F11, or the system (e.g. a window manager). Use it to
    // persist the setting. Not called when the state does not change.
    void set_fullscreen_listener(std::function<void(bool)> listener) { fullscreen_listener_ = std::move(listener); }
    void set_title(std::string_view title);
    void set_integer_scale(bool on);
    void set_logic_step(double seconds) { config_.logic_step = seconds; }
    const AppConfig& config() const { return config_; }

    // Seconds since run() started (real time).
    double time() const;
    // Called with every raw SDL event BEFORE coordinates are converted and before the scene
    // sees it; return true to consume it (used by ImGuiLayer).
    void set_raw_event_hook(std::function<bool(const SDL_Event&)> hook) { raw_hook_ = std::move(hook); }

    Renderer& renderer() { return *renderer_; }
    Input& input() { return input_; }
    Audio& audio() { return audio_; }
    SDL_Window* window() const { return window_; }
    SDL_Renderer* sdl_renderer() const { return sdl_renderer_; }

private:
    void apply_presentation();
    void handle_event(SDL_Event& e);
    void apply_pending();
    void render_frame(double alpha);
    void note_fullscreen(bool on);

    struct Pending {
        enum class Op { Push, Pop, Replace } op;
        std::unique_ptr<Scene> scene;
    };

    AppConfig config_;
    std::string error_;
    SDL_Window* window_ = nullptr;
    SDL_Renderer* sdl_renderer_ = nullptr;
    std::unique_ptr<Renderer> renderer_;
    Input input_;
    Audio audio_;
    std::vector<std::unique_ptr<Scene>> scenes_;
    std::vector<Pending> pending_;
    std::function<bool(const SDL_Event&)> raw_hook_;
    std::function<void(bool)> fullscreen_listener_;
    bool running_ = false;
    int exit_code_ = 0;
    unsigned long long start_ticks_ = 0;
    bool sdl_initialized_ = false;
    int frames_rendered_ = 0;
};

}  // namespace vs
