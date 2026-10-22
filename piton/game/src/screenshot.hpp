#pragma once
// Debug screenshots (see launch_options.hpp): reads back the frame being rendered and saves it
// as PNG. Used only with --shot-scene; never during normal play.

#include <vs/engine/app.hpp>
#include <vs/engine/renderer.hpp>

namespace piton_game {

// Saves the current frame for shot frame `index` (file name gets "-<index>" when the shot has
// several frames). Returns false on failure.
bool save_shot(vs::Renderer& r, int index);

// Helper for scenes: call at the end of render() once the scene is ready. Captures frame
// `next_index()` and quits the app after the last one.
class ShotTaker {
public:
    bool active() const;
    int index() const { return index_; }
    int frames() const;
    float alpha() const;  // sub-tick progress for the current frame: index / frames
    void capture(vs::App& app, vs::Renderer& r);

private:
    int index_ = 0;
    bool done_ = false;
};

}  // namespace piton_game
