#include <vs/editor/file_dialogs.hpp>

#include <mutex>
#include <optional>

#include <SDL3/SDL.h>

#include <vs/engine/json_file.hpp>

namespace vs {

struct FileDialogs::State {
    std::mutex mutex;
    bool busy = false;
    std::optional<std::filesystem::path> result;  // set by the SDL callback (any thread)
    Callback callback;
    // Filter strings must stay alive until SDL calls back.
    std::vector<FileFilter> filters;
    std::vector<SDL_DialogFileFilter> sdl_filters;
    std::string location;

    static void SDLCALL on_done(void* userdata, const char* const* files, int /*filter*/) {
        auto* self = static_cast<State*>(userdata);
        std::lock_guard lock(self->mutex);
        if (files && files[0])
            self->result = utf8_to_path(files[0]);
        else
            self->result = std::filesystem::path{};
    }

    void prepare(std::vector<FileFilter> f, const std::filesystem::path& default_location, Callback cb) {
        filters = std::move(f);
        sdl_filters.clear();
        for (const FileFilter& ff : filters) sdl_filters.push_back({ff.name.c_str(), ff.pattern.c_str()});
        location = path_to_utf8(default_location);
        callback = std::move(cb);
        result.reset();
        busy = true;
    }
};

FileDialogs::FileDialogs() : state_(std::make_shared<State>()) {}

// The SDL callback may still be pending at exit; keep the state alive in that case.
FileDialogs::~FileDialogs() {
    if (state_->busy) new std::shared_ptr<State>(state_);  // intentional leak, see above
}

void FileDialogs::open_file(SDL_Window* window, std::vector<FileFilter> filters,
                            const std::filesystem::path& default_location, Callback callback) {
    if (busy()) return;
    state_->prepare(std::move(filters), default_location, std::move(callback));
    SDL_ShowOpenFileDialog(&State::on_done, state_.get(), window, state_->sdl_filters.data(),
                           static_cast<int>(state_->sdl_filters.size()),
                           state_->location.empty() ? nullptr : state_->location.c_str(), false);
}

void FileDialogs::save_file(SDL_Window* window, std::vector<FileFilter> filters,
                            const std::filesystem::path& default_location, Callback callback) {
    if (busy()) return;
    state_->prepare(std::move(filters), default_location, std::move(callback));
    SDL_ShowSaveFileDialog(&State::on_done, state_.get(), window, state_->sdl_filters.data(),
                           static_cast<int>(state_->sdl_filters.size()),
                           state_->location.empty() ? nullptr : state_->location.c_str());
}

bool FileDialogs::busy() const { return state_->busy; }

void FileDialogs::poll() {
    std::optional<std::filesystem::path> result;
    {
        std::lock_guard lock(state_->mutex);
        result.swap(state_->result);
    }
    if (!result) return;
    state_->busy = false;
    Callback cb = std::move(state_->callback);
    if (cb) cb(*result);
}

bool launch_process(const std::vector<std::string>& args) {
    if (args.empty()) return false;
    std::vector<const char*> argv;
    for (const std::string& a : args) argv.push_back(a.c_str());
    argv.push_back(nullptr);
    SDL_Process* p = SDL_CreateProcess(argv.data(), false);
    if (!p) {
        SDL_Log("launch_process: %s", SDL_GetError());
        return false;
    }
    SDL_DestroyProcess(p);  // does not stop the child
    return true;
}

}  // namespace vs
