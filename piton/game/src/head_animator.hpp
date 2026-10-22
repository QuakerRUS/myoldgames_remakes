#pragma once
// The snake head's animation state: tongue flicks (idle loop), the bite after eating, the
// open mouth just before a rabbit, the dead head. Produces a frame name of the right-facing
// set; draw_snake() turns it to the actual direction.

#include <string>

#include <vs/engine/animation.hpp>
#include <vs/engine/sprite_sheet.hpp>

namespace piton_game {

class HeadAnimator {
public:
    void init(const vs::SpriteSheet& snake);
    void update(double dt);
    void eat();   // plays the bite once, then back to the tongue loop
    void die();   // dead head from now on
    void reset();
    void set_mouth_open(bool open) { mouth_open_ = open; }  // a rabbit right in front
    std::string frame() const;

private:
    const vs::SpriteSheet* sheet_ = nullptr;
    vs::AnimationPlayer player_{nullptr};
    bool eating_ = false;
    bool dead_ = false;
    bool mouth_open_ = false;
};

}  // namespace piton_game
