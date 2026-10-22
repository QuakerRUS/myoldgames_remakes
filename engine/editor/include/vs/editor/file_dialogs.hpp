#pragma once
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <vector>

struct SDL_Window;

namespace vs {

struct FileFilter {
    std::string name;     // "Soldier level"
    std::string pattern;  // extensions separated by ';' without dots: "json" or "png;bmp"; "*" = all
};

// Native open/save dialogs (SDL_ShowOpenFileDialog / SDL_ShowSaveFileDialog). The dialogs are
// asynchronous: the callback runs from poll() on the main thread (call poll() every frame).
// The callback receives an empty path when the user cancels. One dialog at a time.
class FileDialogs {
public:
    using Callback = std::function<void(const std::filesystem::path& chosen)>;

    FileDialogs();
    ~FileDialogs();

    void open_file(SDL_Window* window, std::vector<FileFilter> filters, const std::filesystem::path& default_location,
                   Callback callback);
    void save_file(SDL_Window* window, std::vector<FileFilter> filters, const std::filesystem::path& default_location,
                   Callback callback);
    bool busy() const;
    void poll();

private:
    struct State;
    std::shared_ptr<State> state_;
};

// Starts a program without waiting for it (e.g. the game for F5 test-play).
// args[0] is the executable path. Returns false if it could not be started.
bool launch_process(const std::vector<std::string>& args);

}  // namespace vs
