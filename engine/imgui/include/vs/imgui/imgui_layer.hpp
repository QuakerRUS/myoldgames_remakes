#pragma once
#include <filesystem>
#include <string>

#include <imgui.h>

#include <SDL3/SDL_events.h>

struct SDL_Window;
struct SDL_Renderer;

namespace vs {

class App;
class Texture;

struct ImGuiLayerOptions {
    // TTF used for the UI. Empty: C:\Windows\Fonts\segoeui.ttf if present (covers Cyrillic),
    // otherwise ImGui's built-in font (ASCII only).
    std::filesystem::path font_path;
    float font_size = 18.0f;
    bool dark_style = true;
    // Where ImGui keeps window layout; empty = do not save layout.
    std::filesystem::path ini_path;
};

// Dear ImGui (v1.93.0) on SDL3 + SDL_Renderer. Typical use inside an editor Scene:
//   on_enter:  layer.attach(app());                    // init + routes App's raw events to ImGui
//   render:    layer.new_frame(); ...ImGui calls...; layer.render();
// One layer per process. Destroy (or shutdown()) before the App's renderer is destroyed —
// keeping it as a member of the Scene does that automatically.
class ImGuiLayer {
public:
    ImGuiLayer() = default;
    ~ImGuiLayer();
    ImGuiLayer(const ImGuiLayer&) = delete;
    ImGuiLayer& operator=(const ImGuiLayer&) = delete;

    bool init(SDL_Window* window, SDL_Renderer* renderer, const ImGuiLayerOptions& options = {});
    // init() with the App's window and renderer, plus App::set_raw_event_hook(process_event).
    bool attach(App& app, const ImGuiLayerOptions& options = {});
    void shutdown();
    bool initialized() const { return initialized_; }

    // Feeds an event to ImGui. Returns true if ImGui wants it exclusively (mouse over an ImGui
    // window, or keyboard focus in an ImGui widget); quit/window events always return false.
    bool process_event(const SDL_Event& event);
    void new_frame();
    void render();

    bool wants_mouse() const;
    bool wants_keyboard() const;
    bool wants_text_input() const;

    // ImGui texture handle of an engine texture, for ImGui::Image / ImDrawList::AddImage.
    static ImTextureID texture_id(const Texture& texture);

private:
    SDL_Renderer* renderer_ = nullptr;
    App* app_ = nullptr;
    std::string ini_path_;
    bool initialized_ = false;
};

}  // namespace vs
