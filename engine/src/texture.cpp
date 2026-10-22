#include <vs/engine/texture.hpp>

#include <SDL3/SDL.h>

#include <vs/engine/error.hpp>
#include <vs/engine/json_file.hpp>
#include <vs/engine/renderer.hpp>

namespace vs {

namespace {
SDL_ScaleMode to_sdl(ScaleMode mode) { return mode == ScaleMode::Linear ? SDL_SCALEMODE_LINEAR : SDL_SCALEMODE_NEAREST; }
}  // namespace

Texture::~Texture() {
    if (texture_) SDL_DestroyTexture(texture_);
}

Texture::Texture(Texture&& other) noexcept : texture_(other.texture_), width_(other.width_), height_(other.height_) {
    other.texture_ = nullptr;
    other.width_ = other.height_ = 0;
}

Texture& Texture::operator=(Texture&& other) noexcept {
    if (this != &other) {
        if (texture_) SDL_DestroyTexture(texture_);
        texture_ = other.texture_;
        width_ = other.width_;
        height_ = other.height_;
        other.texture_ = nullptr;
        other.width_ = other.height_ = 0;
    }
    return *this;
}

Texture Texture::load(Renderer& renderer, const std::filesystem::path& path, ScaleMode mode) {
    try {
        return from_image(renderer, load_image(path), mode);
    } catch (const Error& e) {
        throw Error(std::string("texture: ") + e.what());
    }
}

Texture Texture::from_image(Renderer& renderer, const Image& image, ScaleMode mode) {
    if (image.empty()) throw Error("texture: empty image");
    Texture t;
    t.texture_ = SDL_CreateTexture(renderer.sdl(), SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, image.width,
                                   image.height);
    if (!t.texture_) throw Error(std::string("texture: ") + SDL_GetError());
    t.width_ = image.width;
    t.height_ = image.height;
    SDL_SetTextureBlendMode(t.texture_, SDL_BLENDMODE_BLEND);
    SDL_SetTextureScaleMode(t.texture_, to_sdl(mode));
    SDL_UpdateTexture(t.texture_, nullptr, image.pixels.data(), image.width * 4);
    return t;
}

Texture Texture::create_streaming(Renderer& renderer, int width, int height, ScaleMode mode) {
    Texture t;
    t.texture_ = SDL_CreateTexture(renderer.sdl(), SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, width, height);
    if (!t.texture_) throw Error(std::string("texture: ") + SDL_GetError());
    t.width_ = width;
    t.height_ = height;
    SDL_SetTextureBlendMode(t.texture_, SDL_BLENDMODE_BLEND);
    SDL_SetTextureScaleMode(t.texture_, to_sdl(mode));
    return t;
}

void Texture::update(const Image& image) {
    if (!texture_ || image.width != width_ || image.height != height_) throw Error("texture: update size mismatch");
    SDL_UpdateTexture(texture_, nullptr, image.pixels.data(), image.width * 4);
}

void Texture::set_scale_mode(ScaleMode mode) {
    if (texture_) SDL_SetTextureScaleMode(texture_, to_sdl(mode));
}

}  // namespace vs
