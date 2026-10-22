#pragma once
#include <filesystem>
#include <memory>
#include <string>

#include <vs/engine/scene.hpp>
#include <vs/imgui/imgui_layer.hpp>

#include "painter.hpp"

namespace painter {

struct LaunchOptions {
    std::filesystem::path open;        // file to open at start
    std::string frame;                 // frame to show alone (--frame)
    std::filesystem::path selftest;    // run Painter::run_self_test with this scratch dir and quit
};

// Hosts the ImGui layer and the Painter for vs::App.
class PainterScene : public vs::Scene {
public:
    explicit PainterScene(LaunchOptions options) : options_(std::move(options)) {}
    void on_enter() override;
    void update(double dt) override;
    void render(vs::Renderer& r, double alpha) override;
    bool on_quit_request() override;

private:
    LaunchOptions options_;
    vs::ImGuiLayer layer_;
    std::unique_ptr<Painter> painter_;  // declared after layer_: destroyed first
    int frames_ = 0;
};

}  // namespace painter
