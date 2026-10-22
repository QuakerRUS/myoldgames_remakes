#include <vs/engine/image.hpp>

#include <cstring>
#include <string>

#include <vs/engine/error.hpp>
#include <vs/engine/json_file.hpp>
#include "stb.hpp"

namespace vs {

Image::Image(int w, int h, Color fill) : width(w), height(h) {
    pixels.resize(static_cast<std::size_t>(w > 0 ? w : 0) * static_cast<std::size_t>(h > 0 ? h : 0) * 4);
    for (std::size_t i = 0; i < pixels.size(); i += 4) {
        pixels[i] = fill.r;
        pixels[i + 1] = fill.g;
        pixels[i + 2] = fill.b;
        pixels[i + 3] = fill.a;
    }
}

Color Image::get(int x, int y) const {
    const std::size_t i = (static_cast<std::size_t>(y) * static_cast<std::size_t>(width) + static_cast<std::size_t>(x)) * 4;
    return {pixels[i], pixels[i + 1], pixels[i + 2], pixels[i + 3]};
}

void Image::set(int x, int y, Color c) {
    const std::size_t i = (static_cast<std::size_t>(y) * static_cast<std::size_t>(width) + static_cast<std::size_t>(x)) * 4;
    pixels[i] = c.r;
    pixels[i + 1] = c.g;
    pixels[i + 2] = c.b;
    pixels[i + 3] = c.a;
}

Image decode_image(const void* data, std::size_t size) {
    int w = 0;
    int h = 0;
    int channels = 0;
    unsigned char* rgba = stb::load_image_rgba(static_cast<const unsigned char*>(data), size, &w, &h, &channels);
    if (!rgba) throw Error(std::string("cannot decode image: ") + stb::image_failure_reason());
    Image img;
    img.width = w;
    img.height = h;
    img.pixels.assign(rgba, rgba + static_cast<std::size_t>(w) * static_cast<std::size_t>(h) * 4);
    stb::free_image(rgba);
    return img;
}

Image load_image(const std::filesystem::path& path) {
    const std::vector<unsigned char> bytes = read_binary_file(path);
    try {
        return decode_image(bytes.data(), bytes.size());
    } catch (const Error& e) {
        throw Error(path_to_utf8(path) + ": " + e.what());
    }
}

std::vector<std::uint8_t> encode_png(const Image& image) {
    if (image.empty()) throw Error("cannot encode an empty image");
    std::vector<std::uint8_t> out = stb::encode_png_rgba(image.pixels.data(), image.width, image.height);
    if (out.empty()) throw Error("PNG encoding failed");
    return out;
}

void save_png(const std::filesystem::path& path, const Image& image) {
    const std::vector<std::uint8_t> png = encode_png(image);
    write_file_atomic(path, std::string_view(reinterpret_cast<const char*>(png.data()), png.size()));
}

}  // namespace vs
