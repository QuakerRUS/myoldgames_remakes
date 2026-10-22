#include "ui_draw.hpp"

#include <algorithm>
#include <string>

#include "assets.hpp"
#include "palette.hpp"

namespace piton_game {

void draw_nine_slice(vs::Renderer& r, const vs::SpriteSheet& sheet, std::string_view name, vs::RectF dst, int border,
                     float scale) {
    const vs::FrameRect* f = sheet.frame(name);
    if (!f) return;
    const float b = static_cast<float>(border);
    const float sx[4] = {0.0f, b, static_cast<float>(f->w) - b, static_cast<float>(f->w)};
    const float sy[4] = {0.0f, b, static_cast<float>(f->h) - b, static_cast<float>(f->h)};
    const float db = b * scale;
    const float dx[4] = {dst.x, dst.x + db, dst.right() - db, dst.right()};
    const float dy[4] = {dst.y, dst.y + db, dst.bottom() - db, dst.bottom()};
    for (int j = 0; j < 3; ++j) {
        for (int i = 0; i < 3; ++i) {
            const vs::RectF src{static_cast<float>(f->x) + sx[i], static_cast<float>(f->y) + sy[j], sx[i + 1] - sx[i],
                                sy[j + 1] - sy[j]};
            const vs::RectF out{dx[i], dy[j], dx[i + 1] - dx[i], dy[j + 1] - dy[j]};
            if (out.w > 0 && out.h > 0) r.draw_stretched(sheet.texture(), src, out);
        }
    }
}

void draw_wood_frame(vs::Renderer& r, const Assets& a, vs::RectF inner, float thickness) {
    const vs::RectF outer{inner.x - thickness, inner.y - thickness, inner.w + 2 * thickness, inner.h + 2 * thickness};
    r.fill_rect({outer.x + 6, outer.y + 8, outer.w, outer.h}, vs::rgba(0x00000070));
    // The panel frame has an 8 px border; draw it twice as large for a sturdy frame and
    // let the field cover the centre.
    draw_nine_slice(r, a.ui, "panel", outer, 8, thickness / 8.0f);
}

void draw_panel(vs::Renderer& r, const Assets& a, vs::RectF rect) {
    r.fill_rect({rect.x + 6, rect.y + 8, rect.w, rect.h}, vs::rgba(0x00000060));
    draw_nine_slice(r, a.ui, "panel", rect, 8, 2.0f);
    r.fill_rect(rect.inset(16), pal::kPanel);
}

vs::Vec2 draw_text(vs::Renderer& r, const vs::Font& f, std::string_view text, vs::Vec2 pos, vs::Color color,
                   vs::Align align, TextFx fx) {
    vs::TextStyle st;
    st.color = color;
    st.align = align;
    const float unit = std::max(1.0f, std::floor(f.size() / 8.0f));
    if (fx == TextFx::Shadow || fx == TextFx::Both) {
        st.shadow = vs::Color{0, 0, 0, static_cast<std::uint8_t>(color.a * 3 / 4)};
        st.shadow_offset = {unit, unit};
    }
    if (fx == TextFx::Outline || fx == TextFx::Both) {
        st.outline = pal::kOutline.with_alpha(color.a);
        st.outline_width = unit;
    }
    return f.draw(r, text, pos, st);
}

void draw_backdrop(vs::Renderer& r, const Assets& a, float dim) {
    r.draw(a.menu_bg, std::nullopt, {0, 0});
    if (dim > 0) r.fill_rect({0, 0, 1280, 720}, vs::Color{0, 0, 0, static_cast<std::uint8_t>(std::clamp(dim, 0.0f, 1.0f) * 255)});
}

std::string padded(int value, int digits) {
    std::string s = std::to_string(std::max(0, value));
    if (static_cast<int>(s.size()) < digits) s.insert(0, static_cast<std::size_t>(digits) - s.size(), '0');
    return s;
}

}  // namespace piton_game
