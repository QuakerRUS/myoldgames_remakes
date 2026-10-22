#include <doctest/doctest.h>
#include <vs/engine/camera.hpp>

TEST_CASE("camera clamps to world bounds") {
    vs::Camera2D cam;
    cam.viewport = {0, 0, 100, 100};
    cam.set_bounds({0, 0, 1000, 500});
    cam.center_on({10, 10});
    CHECK(cam.position == vs::Vec2{0, 0});
    cam.center_on({990, 490});
    CHECK(cam.position == vs::Vec2{900, 400});
    cam.center_on({500, 250});
    CHECK(cam.position == vs::Vec2{450, 200});
}

TEST_CASE("camera centres a world smaller than the view") {
    vs::Camera2D cam;
    cam.viewport = {0, 0, 200, 100};
    cam.set_bounds({0, 0, 100, 400});
    cam.center_on({0, 0});
    CHECK(cam.position.x == doctest::Approx(-50));  // centred horizontally
    CHECK(cam.position.y == doctest::Approx(0));
}

TEST_CASE("camera zoom changes the view size") {
    vs::Camera2D cam;
    cam.viewport = {0, 0, 640, 320};
    cam.zoom = 2;
    CHECK(cam.view_size() == vs::Vec2{320, 160});
    cam.set_bounds({0, 0, 1000, 1000});
    cam.center_on({1000, 1000});
    CHECK(cam.position == vs::Vec2{680, 840});
}

TEST_CASE("camera world/screen conversion with viewport offset") {
    vs::Camera2D cam;
    cam.viewport = {100, 50, 400, 300};
    cam.position = {10, 20};
    cam.zoom = 2;
    const vs::Vec2 s = cam.world_to_screen({15, 25});
    CHECK(s == vs::Vec2{110, 60});
    CHECK(cam.screen_to_world(s) == vs::Vec2{15, 25});
}

TEST_CASE("camera follow converges and stays clamped") {
    vs::Camera2D cam;
    cam.viewport = {0, 0, 100, 100};
    cam.set_bounds({0, 0, 1000, 1000});
    for (int i = 0; i < 600; ++i) cam.follow({500, 500}, 1.0 / 60.0);
    CHECK(cam.position.x == doctest::Approx(450).epsilon(0.001));
    CHECK(cam.position.y == doctest::Approx(450).epsilon(0.001));
    for (int i = 0; i < 600; ++i) cam.follow({2000, -50}, 1.0 / 60.0);
    CHECK(cam.position.x == doctest::Approx(900));
    CHECK(cam.position.y == doctest::Approx(0));
}
