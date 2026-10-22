#pragma once
#include <filesystem>

#include <vs/engine/image.hpp>

struct SDL_Texture;

namespace vs {

class Renderer;

enum class ScaleMode {
    Nearest,  // pixel art (default)
    Linear,   // smooth (photos, text)
};

// GPU texture owned by an SDL_Renderer. Move-only RAII handle; must be destroyed before the
// renderer (keep textures inside scenes or objects owned by the App's lifetime).
class Texture {
public:
    Texture() = default;
    ~Texture();
    Texture(Texture&& other) noexcept;
    Texture& operator=(Texture&& other) noexcept;
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    // Loads a PNG (any stb_image format). Throws vs::Error.
    static Texture load(Renderer& renderer, const std::filesystem::path& path, ScaleMode mode = ScaleMode::Nearest);
    // Uploads an RGBA8 image. Throws vs::Error.
    static Texture from_image(Renderer& renderer, const Image& image, ScaleMode mode = ScaleMode::Nearest);
    // Creates a streaming texture that can be updated with update(); for editors' canvases.
    static Texture create_streaming(Renderer& renderer, int width, int height, ScaleMode mode = ScaleMode::Nearest);
    // Replaces the whole contents (image size must equal the texture size). Throws vs::Error.
    void update(const Image& image);

    bool valid() const { return texture_ != nullptr; }
    explicit operator bool() const { return valid(); }
    int width() const { return width_; }
    int height() const { return height_; }
    void set_scale_mode(ScaleMode mode);
    SDL_Texture* sdl() const { return texture_; }

private:
    SDL_Texture* texture_ = nullptr;
    int width_ = 0;
    int height_ = 0;
};

}  // namespace vs
