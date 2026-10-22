#include "head_animator.hpp"

namespace piton_game {

void HeadAnimator::init(const vs::SpriteSheet& snake) {
    sheet_ = &snake;
    reset();
}

void HeadAnimator::reset() {
    eating_ = false;
    dead_ = false;
    mouth_open_ = false;
    if (sheet_) player_.play(sheet_->animation("head_r"), true);
}

void HeadAnimator::update(double dt) {
    player_.update(dt);
    if (eating_ && player_.finished()) {
        eating_ = false;
        player_.play(sheet_->animation("head_r"), true);
    }
}

void HeadAnimator::eat() {
    if (dead_ || !sheet_) return;
    eating_ = true;
    player_.play(sheet_->animation("head_eat_r"), true);
}

void HeadAnimator::die() {
    if (!sheet_) return;
    dead_ = true;
    eating_ = false;
    player_.play(sheet_->animation("head_dead_r"), true);
}

std::string HeadAnimator::frame() const {
    if (dead_) return "head_r_dead";
    if (!eating_ && mouth_open_) return "head_r_open";
    const std::string& f = player_.frame();
    return f.empty() ? std::string("head_r") : f;
}

}  // namespace piton_game
