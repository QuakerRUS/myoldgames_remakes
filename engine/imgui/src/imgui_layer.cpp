#include <vs/imgui/imgui_layer.hpp>

#include <system_error>

#include <SDL3/SDL.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>

#include <vs/engine/app.hpp>
#include <vs/engine/json_file.hpp>
#include <vs/engine/texture.hpp>

namespace vs {

ImGuiLayer::~ImGuiLayer() { shutdown(); }

bool ImGuiLayer::init(SDL_Window* window, SDL_Renderer* renderer, const ImGuiLayerOptions& options) {
    if (initialized_) return true;
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags &= ~ImGuiConfigFlags_NavEnableKeyboard;  // arrows/Tab belong to the editor
    if (options.ini_path.empty()) {
        io.IniFilename = nullptr;
    } else {
        ini_path_ = path_to_utf8(options.ini_path);
        io.IniFilename = ini_path_.c_str();
    }
    if (options.dark_style)
        ImGui::StyleColorsDark();
    else
        ImGui::StyleColorsLight();

    std::filesystem::path font = options.font_path;
    std::error_code ec;
    if (font.empty() && std::filesystem::exists("C:/Windows/Fonts/segoeui.ttf", ec)) font = "C:/Windows/Fonts/segoeui.ttf";
    if (!font.empty()) {
        const std::string utf8 = path_to_utf8(font);
        if (!io.Fonts->AddFontFromFileTTF(utf8.c_str(), options.font_size)) io.Fonts->AddFontDefault();
    }
    if (!ImGui_ImplSDL3_InitForSDLRenderer(window, renderer) || !ImGui_ImplSDLRenderer3_Init(renderer)) {
        ImGui::DestroyContext();
        return false;
    }
    renderer_ = renderer;
    initialized_ = true;
    return true;
}

bool ImGuiLayer::attach(App& app, const ImGuiLayerOptions& options) {
    if (!init(app.window(), app.sdl_renderer(), options)) return false;
    app_ = &app;
    app.set_raw_event_hook([this](const SDL_Event& e) { return process_event(e); });
    return true;
}

void ImGuiLayer::shutdown() {
    if (!initialized_) return;
    if (app_) app_->set_raw_event_hook({});
    app_ = nullptr;
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    initialized_ = false;
}

// Returns true (consumed) for input ImGui wants. App still feeds consumed key and mouse-button
// releases to vs::Input, so nothing stays "down" when a release lands over an ImGui window.
bool ImGuiLayer::process_event(const SDL_Event& e) {
    if (!initialized_) return false;
    ImGui_ImplSDL3_ProcessEvent(&e);
    const ImGuiIO& io = ImGui::GetIO();
    switch (e.type) {
    case SDL_EVENT_MOUSE_MOTION:
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP:
    case SDL_EVENT_MOUSE_WHEEL:
        return io.WantCaptureMouse;
    case SDL_EVENT_KEY_DOWN:
    case SDL_EVENT_KEY_UP:
    case SDL_EVENT_TEXT_INPUT:
        return io.WantCaptureKeyboard;
    default:
        return false;
    }
}

void ImGuiLayer::new_frame() {
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
}

void ImGuiLayer::render() {
    ImGui::Render();
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer_);
}

bool ImGuiLayer::wants_mouse() const { return initialized_ && ImGui::GetIO().WantCaptureMouse; }
bool ImGuiLayer::wants_keyboard() const { return initialized_ && ImGui::GetIO().WantCaptureKeyboard; }
bool ImGuiLayer::wants_text_input() const { return initialized_ && ImGui::GetIO().WantTextInput; }

ImTextureID ImGuiLayer::texture_id(const Texture& texture) {
    return static_cast<ImTextureID>(reinterpret_cast<intptr_t>(texture.sdl()));
}

}  // namespace vs
