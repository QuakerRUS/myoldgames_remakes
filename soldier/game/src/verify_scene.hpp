// --verify-replays: plays every built-in LISIY record through ReplayScene at high speed
// and prints whether each one ends in a win. Exits with 0 only if all of them do.
#pragma once

#include <vector>

#include "base_scene.hpp"
#include "replay_scene.hpp"

namespace sg {

class VerifyScene : public BaseScene {
public:
    explicit VerifyScene(ResourcesPtr res) : BaseScene(std::move(res)) {}
    void on_enter() override;

protected:
    void tick(double) override {}
    void draw(vs::Renderer& r) override;
    bool screenshot_due() const override { return false; }

private:
    int next_ = 1;
    int failures_ = 0;
    std::vector<ReplayResult> results_;
    void finish();
};

}  // namespace sg
