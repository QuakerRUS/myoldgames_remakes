#include <doctest/doctest.h>
#include <vs/engine/animation.hpp>

namespace {
vs::AnimationDef make(bool loop) {
    vs::AnimationDef a;
    a.name = "a";
    a.loop = loop;
    a.frames = {{"f0", 100}, {"f1", 50}};
    return a;
}
}  // namespace

TEST_CASE("animation advances through frames and loops") {
    const vs::AnimationDef def = make(true);
    vs::AnimationPlayer p(&def);
    CHECK(p.frame() == "f0");
    p.update(0.099);
    CHECK(p.frame() == "f0");
    p.update(0.002);
    CHECK(p.frame() == "f1");
    p.update(0.050);  // 151 ms -> wraps to 1 ms
    CHECK(p.frame() == "f0");
    CHECK_FALSE(p.finished());
}

TEST_CASE("non-looping animation stops on its last frame") {
    const vs::AnimationDef def = make(false);
    vs::AnimationPlayer p(&def);
    p.update(1.0);
    CHECK(p.finished());
    CHECK(p.frame() == "f1");
    p.play(&def);  // same animation, no restart
    CHECK(p.finished());
    p.play(&def, true);
    CHECK_FALSE(p.finished());
    CHECK(p.frame() == "f0");
}

TEST_CASE("animation speed scales time") {
    const vs::AnimationDef def = make(true);
    vs::AnimationPlayer p(&def);
    p.set_speed(2.0);
    p.update(0.06);  // 120 ms
    CHECK(p.frame() == "f1");
    vs::AnimationPlayer empty;
    CHECK(empty.frame().empty());
}
