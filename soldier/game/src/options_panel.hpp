// The OPTIONS dialog of the main menu: game speed, sound and music volume, fullscreen.
#pragma once

#include <functional>

#include <vs/engine/app.hpp>

#include "resources.hpp"

namespace sg {

class OptionsPanel {
public:
    void open();
    bool is_open() const { return open_; }
    // Handles the keys while open; returns true while open.
    bool update(vs::App& app, Resources& res, double dt);
    void render(vs::Renderer& r, const Resources& res, const vs::App& app, double time) const;

private:
    enum Row { Speed, Sound, Music, Zoom, Fullscreen, Back, RowCount };
    bool open_ = false;
    int row_ = 0;
    void adjust(vs::App& app, Resources& res, int dir);
    vs::RectF row_rect(int i) const;
};

}  // namespace sg
