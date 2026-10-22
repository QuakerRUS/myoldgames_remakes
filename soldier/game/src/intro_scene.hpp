// Company intro ("V Software Entertainment presents") and the SOLDIER title card.
// Any key (or mouse button) skips to the main menu.
#pragma once

#include <vs/engine/texture.hpp>

#include "base_scene.hpp"

namespace sg {

class IntroScene : public BaseScene {
public:
    explicit IntroScene(ResourcesPtr res);
    void on_enter() override;

protected:
    void tick(double dt) override;
    void draw(vs::Renderer& r) override;

private:
    vs::Texture emblem_, software_, rule_, entertainment_, presents_;
    bool leaving_ = false;

    void draw_company(vs::Renderer& r, double t);
    void draw_title(vs::Renderer& r, double t);
    void leave();
};

}  // namespace sg
