#pragma once
// The rabbit on the field: pops in with a hop when it appears, then idles (nose, ears,
// looking around, an occasional hop on the spot). The eaten rabbit stays visible until the
// head has slid over it.

#include <optional>
#include <string>

#include <vs/engine/animation.hpp>
#include <vs/engine/renderer.hpp>
#include <vs/engine/sprite_sheet.hpp>

#include <piton/types.hpp>

namespace piton_game {

class RabbitView {
public:
    void init(const vs::SpriteSheet& rabbit);
    void spawn(piton::Cell c);            // a new rabbit appeared
    void eaten(double linger_seconds);    // current rabbit eaten: keep it for a moment
    void clear();
    void update(double dt);
    // Draws with cell (0,0) at `origin`.
    void draw(vs::Renderer& r, vs::Vec2 origin) const;
    std::optional<piton::Cell> cell() const { return cell_; }

private:
    const vs::SpriteSheet* sheet_ = nullptr;
    vs::AnimationPlayer anim_{nullptr};
    std::optional<piton::Cell> cell_;
    bool flip_ = false;
    double age_ = 0;
    std::optional<piton::Cell> eaten_cell_;
    bool eaten_flip_ = false;
    double eaten_left_ = 0;
    std::string eaten_frame_;
};

}  // namespace piton_game
