#pragma once
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <vector>

#include <vs/engine/math.hpp>

namespace vs {

// CPU-side RGBA8 image (row-major, top row first, 4 bytes per pixel, not premultiplied).
// SDL-free; used by Painter and for procedurally generated textures.
struct Image {
    int width = 0;
    int height = 0;
    std::vector<std::uint8_t> pixels;  // width * height * 4 bytes

    Image() = default;
    Image(int w, int h, Color fill = {0, 0, 0, 0});
    bool empty() const { return width <= 0 || height <= 0; }
    bool in_bounds(int x, int y) const { return x >= 0 && y >= 0 && x < width && y < height; }
    // Precondition: in_bounds(x, y).
    Color get(int x, int y) const;
    void set(int x, int y, Color c);
};

// Loads a PNG (or any format stb_image reads) as RGBA8. Throws vs::Error.
Image load_image(const std::filesystem::path& path);
// Decodes an image file held in memory. Throws vs::Error.
Image decode_image(const void* data, std::size_t size);
// Encodes as PNG in memory. Throws vs::Error.
std::vector<std::uint8_t> encode_png(const Image& image);
// Writes a PNG file atomically. Throws vs::Error.
void save_png(const std::filesystem::path& path, const Image& image);

}  // namespace vs
