#include <doctest/doctest.h>
#include <vs/engine/error.hpp>
#include <vs/engine/image.hpp>

#include <filesystem>

TEST_CASE("PNG encode/decode round trip keeps RGBA exactly") {
    vs::Image img(3, 2, {0, 0, 0, 0});
    img.set(0, 0, {255, 0, 0, 255});
    img.set(2, 1, {10, 20, 30, 128});
    const auto png = vs::encode_png(img);
    const vs::Image back = vs::decode_image(png.data(), png.size());
    CHECK(back.width == 3);
    CHECK(back.height == 2);
    CHECK(back.get(0, 0) == vs::Color{255, 0, 0, 255});
    CHECK(back.get(2, 1) == vs::Color{10, 20, 30, 128});
    CHECK(back.get(1, 1) == vs::Color{0, 0, 0, 0});
}

TEST_CASE("PNG file save and load") {
    const auto file = std::filesystem::temp_directory_path() / "vs_engine_tests" / "img.png";
    vs::Image img(4, 4, {1, 2, 3, 255});
    vs::save_png(file, img);
    const vs::Image back = vs::load_image(file);
    CHECK(back.get(3, 3) == vs::Color{1, 2, 3, 255});
    CHECK_THROWS_AS(vs::decode_image("nope", 4), vs::Error);
}
