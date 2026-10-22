#include <vs/engine/sprite_sheet.hpp>

#include <vs/engine/error.hpp>
#include <vs/engine/json_file.hpp>

namespace vs {

SpriteSheet SpriteSheet::load(Renderer& renderer, const std::filesystem::path& json_path, ScaleMode mode) {
    SpriteSheetData data = load_sprite_sheet_data(json_path);
    Texture texture = Texture::load(renderer, json_path.parent_path() / utf8_to_path(data.image), mode);
    for (const FrameRect& f : data.frames)
        if (f.x < 0 || f.y < 0 || f.x + f.w > texture.width() || f.y + f.h > texture.height())
            throw Error(path_to_utf8(json_path) + ": frame \"" + f.name + "\" lies outside the image");
    return SpriteSheet(std::move(texture), std::move(data));
}

void SpriteSheet::draw(Renderer& renderer, std::string_view name, Vec2 pos, const DrawParams& params) const {
    const FrameRect* f = data_.find_frame(name);
    if (!f) return;
    DrawParams p = params;
    if (!p.origin) p.origin = Vec2{static_cast<float>(f->pivot_x), static_cast<float>(f->pivot_y)};
    renderer.draw(texture_,
                  RectF{static_cast<float>(f->x), static_cast<float>(f->y), static_cast<float>(f->w),
                        static_cast<float>(f->h)},
                  pos, p);
}

void SpriteSheet::draw(Renderer& renderer, const AnimationPlayer& player, Vec2 pos, const DrawParams& params) const {
    if (player.frame().empty()) return;
    draw(renderer, player.frame(), pos, params);
}

void SpriteSheet::draw_stretched(Renderer& renderer, std::string_view name, RectF dst, Color tint) const {
    const FrameRect* f = data_.find_frame(name);
    if (!f) return;
    renderer.draw_stretched(
        texture_,
        RectF{static_cast<float>(f->x), static_cast<float>(f->y), static_cast<float>(f->w), static_cast<float>(f->h)},
        dst, tint);
}

}  // namespace vs
