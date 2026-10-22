#include "painter_scene.hpp"

#include <stdexcept>

#include <SDL3/SDL.h>

#include <vs/engine/app.hpp>
#include <vs/engine/renderer.hpp>

#include "ui_util.hpp"

namespace painter {

void PainterScene::on_enter() {
    if (painter_) return;
    vs::ImGuiLayerOptions opts;
    opts.font_size = 17;
    if (!layer_.attach(app(), opts)) throw std::runtime_error("ImGui initialization failed");
    apply_painter_style();
    painter_ = std::make_unique<Painter>(app().renderer(), app().window());
    if (!options_.open.empty() && painter_->open(options_.open) && !options_.frame.empty()) painter_->show_frame(options_.frame);
}

void PainterScene::update(double) {
    if (painter_ && painter_->quit_requested()) app().quit();
}

void PainterScene::render(vs::Renderer& r, double) {
    r.clear({30, 32, 37, 255});
    layer_.new_frame();
    painter_->draw();
    if (!options_.selftest.empty() && frames_ == 3) {
        std::string log;
        const bool ok = painter_->run_self_test(options_.selftest, log);
        SDL_Log("%s", log.c_str());
        app().quit(ok ? 0 : 1);
    }
    layer_.render();
    ++frames_;
}

bool PainterScene::on_quit_request() { return !painter_ || painter_->request_quit(); }

}  // namespace painter
