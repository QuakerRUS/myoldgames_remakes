#pragma once
// Decoration of the main menu, after the original (a snake on the grass heading for a rabbit
// around the menu box): a grass lawn whose outer ring is a track; a snake crawls round it and
// chases a rabbit that hops away whenever the snake comes close.

#include <string>
#include <vector>

#include <vs/engine/animation.hpp>
#include <vs/engine/renderer.hpp>

#include <piton/types.hpp>

#include "head_animator.hpp"

namespace piton_game {

struct Assets;

class MenuSnake {
public:
    // A lawn of cols x rows cells with its top-left corner at `origin`.
    void init(const Assets& a, vs::Vec2 origin, int cols, int rows);
    void update(double dt);
    void draw_lawn(vs::Renderer& r, const Assets& a) const;
    void draw_actors(vs::Renderer& r, const Assets& a) const;

private:
    piton::Cell track(int index) const;  // index wraps around
    int ring_size() const { return static_cast<int>(ring_.size()); }

    vs::Vec2 origin_{};
    int cols_ = 0, rows_ = 0;
    std::vector<piton::Cell> ring_;   // clockwise from the top-left corner
    std::vector<std::string> grass_;  // grass frame per lawn cell

    int head_ = 0;          // track index of the head
    double step_time_ = 0;  // time since the last step
    HeadAnimator head_anim_;

    int rabbit_ = 0;          // track index of the rabbit (start of the hop while hopping)
    double hop_time_ = -1;    // < 0: sitting, else seconds into the hop
    bool rabbit_flip_ = false;
    vs::AnimationPlayer rabbit_anim_{nullptr};
};

}  // namespace piton_game
