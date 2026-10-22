#include <vs/engine/screenshot.hpp>

#include <cstring>
#include <exception>

#include <SDL3/SDL.h>

#include <vs/engine/renderer.hpp>
#include <vs/engine/storage.hpp>

namespace vs {

std::optional<Image> read_frame(Renderer& renderer, bool opaque) {
    SDL_Surface* raw = SDL_RenderReadPixels(renderer.sdl(), nullptr);
    if (!raw) return std::nullopt;
    SDL_Surface* rgba = SDL_ConvertSurface(raw, SDL_PIXELFORMAT_RGBA32);
    SDL_DestroySurface(raw);
    if (!rgba) return std::nullopt;
    Image img(rgba->w, rgba->h);
    const std::size_t row = static_cast<std::size_t>(rgba->w) * 4;
    for (int y = 0; y < rgba->h; ++y)
        std::memcpy(&img.pixels[static_cast<std::size_t>(y) * row],
                    static_cast<const unsigned char*>(rgba->pixels) + static_cast<std::ptrdiff_t>(y) * rgba->pitch, row);
    SDL_DestroySurface(rgba);
    if (opaque)
        for (std::size_t i = 3; i < img.pixels.size(); i += 4) img.pixels[i] = 255;
    return img;
}

bool save_frame_png(Renderer& renderer, const std::filesystem::path& path, bool opaque) {
    const std::optional<Image> img = read_frame(renderer, opaque);
    if (!img) {
        SDL_Log("screenshot failed: %s: %s", path_to_utf8(path).c_str(), SDL_GetError());
        return false;
    }
    try {
        save_png(path, *img);
    } catch (const std::exception& e) {
        SDL_Log("screenshot failed: %s", e.what());
        return false;
    }
    return true;
}

}  // namespace vs
