#include "screenshot.hpp"

#include <string>

#include <vs/engine/screenshot.hpp>
#include <vs/engine/storage.hpp>

#include "launch_options.hpp"

namespace piton_game {

bool save_shot(vs::Renderer& r, int index) {
    const ShotOptions& shot = launch_options().shot;
    std::string path = shot.out;
    if (shot.frames > 1) {
        const std::size_t dot = path.rfind('.');
        const std::string suffix = "-" + std::to_string(index);
        if (dot == std::string::npos)
            path += suffix + ".png";
        else
            path.insert(dot, suffix);
    }
    return vs::save_frame_png(r, vs::utf8_to_path(path));  // logs failures
}

bool ShotTaker::active() const { return launch_options().shot.active() && !done_; }

int ShotTaker::frames() const { return launch_options().shot.frames; }

float ShotTaker::alpha() const { return static_cast<float>(index_) / static_cast<float>(frames()); }

void ShotTaker::capture(vs::App& app, vs::Renderer& r) {
    if (!active()) return;
    save_shot(r, index_);
    if (++index_ >= frames()) {
        done_ = true;
        app.quit(0);
    }
}

}  // namespace piton_game
