#include <vs/engine/app.hpp>

#include <algorithm>
#include <cstring>
#include <cmath>
#include <exception>

#include <SDL3/SDL.h>

#include <vs/engine/screenshot.hpp>
#include <vs/engine/storage.hpp>

namespace vs {

std::vector<std::string> parse_common_args(int argc, char** argv, AppConfig& config) {
    std::vector<std::string> rest;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--smoke") {
            config.quit_after = 1.0;
            config.smoke = true;
        } else if (arg == "--data-dir" && i + 1 < argc)
            config.data_dir = utf8_to_path(argv[++i]);
        else if (arg.rfind("--data-dir=", 0) == 0 && arg.size() > 11)
            config.data_dir = utf8_to_path(std::string_view(arg).substr(11));
        else if (arg == "--windowed")
            config.fullscreen = false;
        else if (arg == "--fullscreen")
            config.fullscreen = true;
        else
            rest.push_back(arg);
    }
    if (!config.data_dir.empty()) set_user_data_dir_override(config.data_dir);
    if (config.smoke) set_user_data_smoke_mode(true);
    return rest;
}

App::App(AppConfig config) : config_(std::move(config)) {}

App::~App() {
    // Scenes own textures and fonts: destroy them while the renderer still exists.
    pending_.clear();
    while (!scenes_.empty()) scenes_.pop_back();
    audio_.shutdown();
    renderer_.reset();
    if (sdl_renderer_) SDL_DestroyRenderer(sdl_renderer_);
    if (window_) SDL_DestroyWindow(window_);
    if (sdl_initialized_) SDL_Quit();
}

bool App::init() {
    SDL_SetHint(SDL_HINT_APP_NAME, config_.title.c_str());
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        error_ = std::string("SDL_Init: ") + SDL_GetError();
        return false;
    }
    sdl_initialized_ = true;
    SDL_WindowFlags flags = SDL_WINDOW_HIGH_PIXEL_DENSITY;
    if (config_.resizable) flags |= SDL_WINDOW_RESIZABLE;
    if (config_.fullscreen) flags |= SDL_WINDOW_FULLSCREEN;
    window_ = SDL_CreateWindow(config_.title.c_str(), config_.window_width, config_.window_height, flags);
    if (!window_) {
        error_ = std::string("SDL_CreateWindow: ") + SDL_GetError();
        return false;
    }
    sdl_renderer_ = SDL_CreateRenderer(window_, nullptr);
    if (!sdl_renderer_) {
        error_ = std::string("SDL_CreateRenderer: ") + SDL_GetError();
        return false;
    }
    if (config_.vsync && !SDL_SetRenderVSync(sdl_renderer_, 1)) config_.vsync = false;
    renderer_ = std::make_unique<Renderer>(sdl_renderer_);
    if (config_.logical_width > 0 && config_.logical_height > 0)
        SDL_SetWindowMinimumSize(window_, config_.logical_width / 4, config_.logical_height / 4);
    apply_presentation();
    input_.attach(window_);
    if (config_.enable_audio) audio_.init();
    return true;
}

void App::apply_presentation() {
    if (!sdl_renderer_) return;
    if (config_.logical_width <= 0 || config_.logical_height <= 0) {
        SDL_SetRenderLogicalPresentation(sdl_renderer_, 0, 0, SDL_LOGICAL_PRESENTATION_DISABLED);
        return;
    }
    SDL_SetRenderLogicalPresentation(
        sdl_renderer_, config_.logical_width, config_.logical_height,
        config_.integer_scale ? SDL_LOGICAL_PRESENTATION_INTEGER_SCALE : SDL_LOGICAL_PRESENTATION_LETTERBOX);
}

void App::set_integer_scale(bool on) {
    config_.integer_scale = on;
    apply_presentation();
}

void App::set_fullscreen(bool on) {
    if (window_) SDL_SetWindowFullscreen(window_, on);
    note_fullscreen(on);
}

void App::note_fullscreen(bool on) {
    if (config_.fullscreen == on) return;
    config_.fullscreen = on;
    if (fullscreen_listener_) fullscreen_listener_(on);
}

bool App::fullscreen() const {
    if (!window_) return config_.fullscreen;
    return (SDL_GetWindowFlags(window_) & SDL_WINDOW_FULLSCREEN) != 0;
}

void App::set_title(std::string_view title) {
    config_.title = std::string(title);
    if (window_) SDL_SetWindowTitle(window_, config_.title.c_str());
}

double App::time() const { return static_cast<double>(SDL_GetTicksNS() - start_ticks_) / 1e9; }

void App::quit(int exit_code) {
    exit_code_ = exit_code;
    running_ = false;
}

void App::push_scene(std::unique_ptr<Scene> scene) {
    if (scene) pending_.push_back({Pending::Op::Push, std::move(scene)});
}

void App::pop_scene() { pending_.push_back({Pending::Op::Pop, nullptr}); }

void App::replace_scene(std::unique_ptr<Scene> scene) {
    if (scene) pending_.push_back({Pending::Op::Replace, std::move(scene)});
}

Scene* App::top_scene() const { return scenes_.empty() ? nullptr : scenes_.back().get(); }

void App::apply_pending() {
    while (!pending_.empty()) {
        Pending p = std::move(pending_.front());
        pending_.erase(pending_.begin());
        if (p.op == Pending::Op::Pop || p.op == Pending::Op::Replace) {
            if (!scenes_.empty()) {
                scenes_.back()->on_exit();
                scenes_.pop_back();
            }
        } else if (!scenes_.empty()) {
            scenes_.back()->on_exit();
        }
        if (p.scene) {
            p.scene->app_ = this;
            scenes_.push_back(std::move(p.scene));
        }
        if (!scenes_.empty()) scenes_.back()->on_enter();
    }
}

void App::handle_event(SDL_Event& e) {
    // Window close: SDL turns the (only) window's close request (title bar X, Alt+F4) into
    // SDL_EVENT_QUIT, which always goes to the top scene's on_quit_request(), even if a raw
    // hook (ImGui) claims it. SDL_EVENT_WINDOW_CLOSE_REQUESTED itself is not handled, so the
    // scene is asked exactly once.
    const bool quit_event = e.type == SDL_EVENT_QUIT;
    if (raw_hook_ && raw_hook_(e) && !quit_event) {
        // A consumed release must still reach Input, or a key/button pressed outside ImGui and
        // released over it would stay "down" forever.
        if (e.type == SDL_EVENT_KEY_UP || e.type == SDL_EVENT_MOUSE_BUTTON_UP) {
            SDL_ConvertEventToRenderCoordinates(sdl_renderer_, &e);
            input_.handle_event(e);
        }
        return;
    }
    SDL_ConvertEventToRenderCoordinates(sdl_renderer_, &e);
    if (e.type == SDL_EVENT_WINDOW_ENTER_FULLSCREEN || e.type == SDL_EVENT_WINDOW_LEAVE_FULLSCREEN)
        note_fullscreen(e.type == SDL_EVENT_WINDOW_ENTER_FULLSCREEN);
    if (quit_event) {
        Scene* top = top_scene();
        if (!top || top->on_quit_request()) quit(exit_code_);
        return;
    }
    if (config_.fullscreen_keys && e.type == SDL_EVENT_KEY_DOWN && !e.key.repeat) {
        const bool alt_enter = (e.key.key == SDLK_RETURN || e.key.key == SDLK_KP_ENTER) && (e.key.mod & SDL_KMOD_ALT);
        if (alt_enter || e.key.key == SDLK_F11) {
            toggle_fullscreen();
            return;
        }
    }
    input_.handle_event(e);
    if (Scene* top = top_scene()) top->on_event(e);
}

void App::render_frame(double alpha) {
    renderer_->clear(config_.clear_color);
    if (!scenes_.empty()) {
        std::size_t first = scenes_.size() - 1;
        while (first > 0 && scenes_[first]->is_overlay()) --first;
        for (std::size_t i = first; i < scenes_.size(); ++i) scenes_[i]->render(*renderer_, alpha);
    }
    ++frames_rendered_;
    if (!config_.screenshot_path.empty() && running_ && frames_rendered_ == std::max(config_.screenshot_after_frames, 1)) {
        const bool ok = save_frame_png(*renderer_, config_.screenshot_path);
        SDL_Log("screenshot %s: %s", ok ? "saved" : "FAILED", path_to_utf8(config_.screenshot_path).c_str());
        quit(ok ? 0 : 1);
    }
    SDL_RenderPresent(sdl_renderer_);
}

int App::run(std::unique_ptr<Scene> first) {
    if (!window_) return 1;
    start_ticks_ = SDL_GetTicksNS();
    running_ = true;
    try {
        push_scene(std::move(first));
        apply_pending();
        Uint64 prev = SDL_GetTicksNS();
        double acc = 0.0;
        while (running_ && !scenes_.empty()) {
            SDL_Event e;
            while (SDL_PollEvent(&e)) handle_event(e);
            apply_pending();

            const double step = config_.logic_step > 0 ? config_.logic_step : 1.0 / 60.0;
            const Uint64 now = SDL_GetTicksNS();
            acc += std::min(static_cast<double>(now - prev) / 1e9, step * config_.max_steps_per_frame);
            prev = now;
            int steps = 0;
            while (running_ && acc >= step && steps < config_.max_steps_per_frame) {
                if (Scene* top = top_scene()) top->update(step);
                input_.end_step();
                apply_pending();
                acc -= step;
                ++steps;
            }
            if (acc >= step) acc = std::fmod(acc, step);
            if (!running_ || scenes_.empty()) break;
            render_frame(acc / step);
            if (config_.quit_after > 0 && time() >= config_.quit_after) quit(exit_code_);
            if (!config_.vsync) SDL_Delay(1);
        }
    } catch (const std::exception& ex) {
        SDL_Log("fatal: %s", ex.what());
        if (config_.quit_after <= 0)
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, config_.title.c_str(), ex.what(), window_);
        return 1;
    }
    return exit_code_;
}

}  // namespace vs
