#include <vs/engine/window_icon.hpp>

#include <exception>

#include <SDL3/SDL.h>

#include <vs/engine/app.hpp>

namespace vs {

namespace {
SDL_Surface* wrap(const Image& img) {
    // The surface only borrows the pixels; SDL_SetWindowIcon copies them.
    return SDL_CreateSurfaceFrom(img.width, img.height, SDL_PIXELFORMAT_RGBA32,
                                 const_cast<std::uint8_t*>(img.pixels.data()), img.width * 4);
}
}  // namespace

bool set_window_icon(SDL_Window* window, const Image& icon, const std::vector<Image>& alternates) {
    if (!window || icon.empty()) return false;
    SDL_Surface* surface = wrap(icon);
    if (!surface) return false;
    for (const Image& img : alternates) {
        if (img.empty()) continue;
        if (SDL_Surface* alt = wrap(img)) {
            SDL_AddSurfaceAlternateImage(surface, alt);
            SDL_DestroySurface(alt);  // the main surface keeps a reference
        }
    }
    const bool ok = SDL_SetWindowIcon(window, surface);
    SDL_DestroySurface(surface);
    return ok;
}

bool set_window_icon(SDL_Window* window, const std::filesystem::path& png, const std::vector<std::filesystem::path>& alternates) {
    if (!window || png.empty()) return false;
    try {
        const Image main = load_image(png);
        std::vector<Image> alts;
        for (const std::filesystem::path& p : alternates) {
            if (p.empty()) continue;
            try {
                alts.push_back(load_image(p));
            } catch (const std::exception&) {
            }
        }
        return set_window_icon(window, main, alts);
    } catch (const std::exception&) {
        return false;  // no icon is not worth failing over
    }
}

bool set_window_icon(App& app, const std::filesystem::path& png, const std::vector<std::filesystem::path>& alternates) {
    return set_window_icon(app.window(), png, alternates);
}

}  // namespace vs
