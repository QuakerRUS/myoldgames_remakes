#include <vs/engine/animation.hpp>

#include <cmath>

namespace vs {

namespace {
const std::string kEmpty;
}

void AnimationPlayer::play(const AnimationDef* def, bool restart_now) {
    if (def == def_ && !restart_now) return;
    def_ = def;
    restart();
}

void AnimationPlayer::restart() {
    elapsed_ms_ = 0;
    index_ = 0;
    finished_ = false;
}

void AnimationPlayer::update(double dt) {
    if (!def_ || def_->frames.empty() || finished_) return;
    const int total = def_->total_ms();
    if (total <= 0) return;
    elapsed_ms_ += dt * 1000.0 * speed_;
    if (elapsed_ms_ >= total) {
        if (def_->loop) {
            elapsed_ms_ = std::fmod(elapsed_ms_, static_cast<double>(total));
        } else {
            elapsed_ms_ = total;
            finished_ = true;
            index_ = static_cast<int>(def_->frames.size()) - 1;
            return;
        }
    }
    sync_index();
}

void AnimationPlayer::sync_index() {
    double t = elapsed_ms_;
    const int n = static_cast<int>(def_->frames.size());
    for (int i = 0; i < n; ++i) {
        t -= def_->frames[static_cast<std::size_t>(i)].ms;
        if (t < 0) {
            index_ = i;
            return;
        }
    }
    index_ = n - 1;
}

const std::string& AnimationPlayer::frame() const {
    if (!def_ || def_->frames.empty()) return kEmpty;
    return def_->frames[static_cast<std::size_t>(index_)].frame;
}

}  // namespace vs
