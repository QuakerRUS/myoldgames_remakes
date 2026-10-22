#include "rabbit_view.hpp"

#include <algorithm>

#include "field_renderer.hpp"

namespace piton_game {

void RabbitView::init(const vs::SpriteSheet& rabbit) {
    sheet_ = &rabbit;
    clear();
}

void RabbitView::clear() {
    cell_.reset();
    eaten_cell_.reset();
    eaten_left_ = 0;
}

void RabbitView::spawn(piton::Cell c) {
    cell_ = c;
    age_ = 0;
    flip_ = ((c.x * 7 + c.y * 13) % 3) == 0;  // most rabbits look left, some right
    if (sheet_) anim_.play(sheet_->animation("rabbit_hop"), true);
}

void RabbitView::eaten(double linger_seconds) {
    if (!cell_) return;
    eaten_cell_ = cell_;
    eaten_flip_ = flip_;
    eaten_left_ = linger_seconds;
    eaten_frame_ = anim_.frame();
    cell_.reset();
}

void RabbitView::update(double dt) {
    age_ += dt;
    anim_.update(dt);
    if (sheet_ && anim_.finished()) {
        // After the arrival hop: idle loop, started at a varying phase.
        anim_.play(sheet_->animation("rabbit_idle"), true);
    }
    if (eaten_cell_) {
        eaten_left_ -= dt;
        if (eaten_left_ <= 0) eaten_cell_.reset();
    }
}

void RabbitView::draw(vs::Renderer& r, vs::Vec2 origin) const {
    if (!sheet_) return;
    auto pos_of = [&](piton::Cell c) {
        return vs::Vec2{origin.x + static_cast<float>(c.x) * kCell, origin.y + static_cast<float>(c.y) * kCell};
    };
    if (eaten_cell_) {
        sheet_->draw(r, eaten_frame_.empty() ? std::string("rabbit_sit") : eaten_frame_, pos_of(*eaten_cell_),
                     {.flip = eaten_flip_ ? vs::Flip::Horizontal : vs::Flip::None});
    }
    if (!cell_) return;
    // Pop-in: grows from the cell's bottom centre during the first 0.18 s.
    const float s = static_cast<float>(std::clamp(age_ / 0.18, 0.25, 1.0));
    const vs::Vec2 base = pos_of(*cell_);
    vs::DrawParams p;
    p.flip = flip_ ? vs::Flip::Horizontal : vs::Flip::None;
    p.scale = {s, s};
    p.origin = vs::Vec2{12, 22};
    sheet_->draw(r, anim_, {base.x + 12, base.y + 22}, p);
}

}  // namespace piton_game
