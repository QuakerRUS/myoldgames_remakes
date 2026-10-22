#pragma once
#include <filesystem>
#include <string_view>

#include <vs/engine/animation.hpp>
#include <vs/engine/renderer.hpp>
#include <vs/engine/sprite_sheet_data.hpp>
#include <vs/engine/texture.hpp>

namespace vs {

// A texture plus its named frames and animations. Move-only.
class SpriteSheet {
public:
    SpriteSheet() = default;
    SpriteSheet(Texture texture, SpriteSheetData data) : texture_(std::move(texture)), data_(std::move(data)) {}

    // Loads the JSON description and the image it names (relative to the JSON file).
    // Throws vs::Error.
    static SpriteSheet load(Renderer& renderer, const std::filesystem::path& json_path,
                            ScaleMode mode = ScaleMode::Nearest);

    const Texture& texture() const { return texture_; }
    const SpriteSheetData& data() const { return data_; }
    const FrameRect* frame(std::string_view name) const { return data_.find_frame(name); }
    const AnimationDef* animation(std::string_view name) const { return data_.find_animation(name); }

    // Draws a frame so that its pivot (or params.origin, if set) lands on `pos`.
    // Unknown frame names draw nothing.
    void draw(Renderer& renderer, std::string_view frame, Vec2 pos, const DrawParams& params = {}) const;
    // Draws the player's current frame.
    void draw(Renderer& renderer, const AnimationPlayer& player, Vec2 pos, const DrawParams& params = {}) const;
    // Stretches a frame into `dst` (icons, UI).
    void draw_stretched(Renderer& renderer, std::string_view frame, RectF dst, Color tint = {}) const;

private:
    Texture texture_;
    SpriteSheetData data_;
};

}  // namespace vs
