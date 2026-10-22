#include <vs/engine/font.hpp>

#include <algorithm>
#include <cmath>
#include <unordered_map>

#include <SDL3/SDL.h>

#ifdef _MSC_VER
#pragma warning(push, 0)
#endif
#include <stb_truetype.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <vs/engine/error.hpp>
#include <vs/engine/image.hpp>
#include <vs/engine/json_file.hpp>
#include <vs/engine/renderer.hpp>
#include <vs/engine/texture.hpp>
#include <vs/engine/utf8.hpp>

namespace vs {

struct Font::Impl {
    std::vector<unsigned char> ttf;
    stbtt_fontinfo info{};
    float size = 0;
    float oversample = 1;
    bool smooth = true;
    float scale = 0;  // stb scale for `size`
    float ascent = 0;
    float line_height = 0;
    Texture atlas;
    std::unordered_map<char32_t, stbtt_packedchar> glyphs;

    const stbtt_packedchar* glyph(char32_t cp) const {
        auto it = glyphs.find(cp);
        if (it != glyphs.end()) return &it->second;
        it = glyphs.find(U'?');
        return it != glyphs.end() ? &it->second : nullptr;
    }
    float kern(char32_t a, char32_t b) const {
        return static_cast<float>(stbtt_GetCodepointKernAdvance(&info, static_cast<int>(a), static_cast<int>(b))) * scale;
    }
    float advance(const stbtt_packedchar& g) const { return g.xadvance / oversample; }

    float offset_of(std::string_view line, std::size_t byte_offset) const {
        float x = 0;
        char32_t prev = 0;
        std::size_t pos = 0;
        while (pos < line.size() && pos < byte_offset) {
            const char32_t cp = utf8_next(line, pos);
            if (cp == U'\n' || cp == U'\r') continue;
            if (prev) x += kern(prev, cp);
            if (const stbtt_packedchar* g = glyph(cp)) x += advance(*g);
            prev = cp;
        }
        return x;
    }

    float measure(std::string_view text) const {
        float widest = 0;
        std::size_t begin = 0;
        while (true) {
            const std::size_t nl = text.find('\n', begin);
            const std::string_view line =
                text.substr(begin, nl == std::string_view::npos ? std::string_view::npos : nl - begin);
            widest = std::max(widest, offset_of(line, line.size()));
            if (nl == std::string_view::npos) break;
            begin = nl + 1;
        }
        return widest;
    }
};

namespace {

std::vector<char32_t> default_chars() {
    std::vector<char32_t> cps;
    for (char32_t c = 0x20; c <= 0x7E; ++c) cps.push_back(c);
    for (char32_t c = 0xA0; c <= 0xFF; ++c) cps.push_back(c);
    for (char32_t c = 0x400; c <= 0x4FF; ++c) cps.push_back(c);
    for (char32_t c = 0x2010; c <= 0x2027; ++c) cps.push_back(c);  // dashes, quotes, bullet, ellipsis
    for (char32_t c : {U'№', U'₽', U'←', U'↑', U'→', U'↓', U'×', U'‹',
                       U'›', U'▲', U'▼', U'◀', U'▶'})
        cps.push_back(c);
    return cps;
}

}  // namespace

Font::Font() = default;
Font::~Font() = default;
Font::Font(Font&&) noexcept = default;
Font& Font::operator=(Font&&) noexcept = default;

Font Font::load(Renderer& renderer, const std::filesystem::path& ttf_path, float size, const FontOptions& options) {
    auto impl = std::make_unique<Impl>();
    impl->ttf = read_binary_file(ttf_path);
    const std::string name = path_to_utf8(ttf_path);
    if (!stbtt_InitFont(&impl->info, impl->ttf.data(), stbtt_GetFontOffsetForIndex(impl->ttf.data(), 0)))
        throw Error("font: cannot parse " + name);
    impl->size = size;
    impl->oversample = options.oversample > 0 ? options.oversample : 1.0f;
    impl->smooth = options.smooth;
    impl->scale = stbtt_ScaleForPixelHeight(&impl->info, size);
    int ascent = 0, descent = 0, gap = 0;
    stbtt_GetFontVMetrics(&impl->info, &ascent, &descent, &gap);
    impl->ascent = static_cast<float>(ascent) * impl->scale;
    impl->line_height = static_cast<float>(ascent - descent + gap) * impl->scale;

    std::vector<char32_t> chars = default_chars();
    chars.insert(chars.end(), options.extra_chars.begin(), options.extra_chars.end());
    std::vector<int> codepoints;
    for (char32_t c : chars)
        if (stbtt_FindGlyphIndex(&impl->info, static_cast<int>(c)) != 0 || c == U' ')
            codepoints.push_back(static_cast<int>(c));
    std::vector<stbtt_packedchar> packed(codepoints.size());
    stbtt_pack_range range{};
    range.font_size = size * impl->oversample;
    range.array_of_unicode_codepoints = codepoints.data();
    range.num_chars = static_cast<int>(codepoints.size());
    range.chardata_for_range = packed.data();

    for (int dim = 256; dim <= 4096; dim *= 2) {
        std::vector<unsigned char> alpha(static_cast<std::size_t>(dim) * static_cast<std::size_t>(dim));
        stbtt_pack_context ctx;
        if (!stbtt_PackBegin(&ctx, alpha.data(), dim, dim, 0, 1, nullptr)) throw Error("font: pack failed");
        const int ok = stbtt_PackFontRanges(&ctx, impl->ttf.data(), 0, &range, 1);
        stbtt_PackEnd(&ctx);
        if (!ok) continue;
        Image img(dim, dim, {255, 255, 255, 0});
        for (std::size_t i = 0; i < alpha.size(); ++i) img.pixels[i * 4 + 3] = alpha[i];
        impl->atlas = Texture::from_image(renderer, img, impl->smooth ? ScaleMode::Linear : ScaleMode::Nearest);
        for (std::size_t i = 0; i < codepoints.size(); ++i) impl->glyphs[static_cast<char32_t>(codepoints[i])] = packed[i];
        Font font;
        font.impl_ = std::move(impl);
        return font;
    }
    throw Error("font: glyphs do not fit a 4096x4096 atlas: " + name);
}

bool Font::valid() const { return impl_ && impl_->atlas.valid(); }
float Font::size() const { return impl_ ? impl_->size : 0.0f; }
float Font::ascent() const { return impl_ ? impl_->ascent : 0.0f; }
float Font::line_height() const { return impl_ ? impl_->line_height : 0.0f; }

bool Font::has_glyph(char32_t cp) const { return impl_ && impl_->glyphs.count(cp) != 0; }

float Font::offset_of(std::string_view line, std::size_t byte_offset) const {
    return impl_ ? impl_->offset_of(line, byte_offset) : 0.0f;
}

float Font::measure(std::string_view text) const { return impl_ ? impl_->measure(text) : 0.0f; }

MeasureFn Font::measure_fn() const {
    const Impl* impl = impl_.get();
    return [impl](std::string_view s) { return impl ? impl->measure(s) : 0.0f; };
}

std::vector<LineRange> Font::wrap_ranges(std::string_view text, float max_width) const {
    return wrap_text_ranges(text, max_width, measure_fn());
}

std::vector<std::string> Font::wrap(std::string_view text, float max_width) const {
    return wrap_text(text, max_width, measure_fn());
}

Vec2 Font::measure_block(std::string_view text, const TextStyle& style) const {
    const std::vector<LineRange> lines = wrap_ranges(text, style.max_width);
    float w = 0;
    for (const LineRange& r : lines) w = std::max(w, measure(text.substr(r.begin, r.end - r.begin)));
    const float step = line_height() * style.line_spacing;
    const float h = lines.empty() ? 0.0f : step * static_cast<float>(lines.size() - 1) + line_height();
    return {w, h};
}

Vec2 Font::draw(Renderer& renderer, std::string_view text, Vec2 pos, const TextStyle& style) const {
    if (!valid()) return {};
    const Impl& im = *impl_;
    const std::vector<LineRange> lines = wrap_ranges(text, style.max_width);
    const float step = im.line_height * style.line_spacing;
    const float inv_w = 1.0f / static_cast<float>(im.atlas.width());
    const float inv_h = 1.0f / static_cast<float>(im.atlas.height());

    std::vector<SDL_Vertex> verts;
    std::vector<int> indices;
    auto build = [&](Vec2 offset, Color color) {
        verts.clear();
        indices.clear();
        const SDL_FColor fc{color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f};
        float y = pos.y + offset.y;
        for (const LineRange& r : lines) {
            const std::string_view line = text.substr(r.begin, r.end - r.begin);
            float x = pos.x + offset.x;
            if (style.align != Align::Left) {
                const float w = measure(line);
                x -= style.align == Align::Center ? w * 0.5f : w;
            }
            if (!im.smooth) x = std::round(x);
            const float baseline = im.smooth ? y + im.ascent : std::round(y + im.ascent);
            char32_t prev = 0;
            std::size_t p = 0;
            while (p < line.size()) {
                const char32_t cp = utf8_next(line, p);
                if (cp == U'\r') continue;
                if (prev) x += im.kern(prev, cp);
                prev = cp;
                const stbtt_packedchar* g = im.glyph(cp);
                if (!g) continue;
                const float x0 = x + g->xoff / im.oversample, y0 = baseline + g->yoff / im.oversample;
                const float x1 = x + g->xoff2 / im.oversample, y1 = baseline + g->yoff2 / im.oversample;
                const float u0 = g->x0 * inv_w, v0 = g->y0 * inv_h, u1 = g->x1 * inv_w, v1 = g->y1 * inv_h;
                if (x1 > x0 && y1 > y0) {
                    const int base = static_cast<int>(verts.size());
                    verts.push_back({{x0, y0}, fc, {u0, v0}});
                    verts.push_back({{x1, y0}, fc, {u1, v0}});
                    verts.push_back({{x1, y1}, fc, {u1, v1}});
                    verts.push_back({{x0, y1}, fc, {u0, v1}});
                    indices.insert(indices.end(), {base, base + 1, base + 2, base, base + 2, base + 3});
                }
                x += im.advance(*g);
            }
            y += step;
        }
        if (!indices.empty())
            SDL_RenderGeometry(renderer.sdl(), im.atlas.sdl(), verts.data(), static_cast<int>(verts.size()),
                               indices.data(), static_cast<int>(indices.size()));
    };

    SDL_SetTextureColorMod(im.atlas.sdl(), 255, 255, 255);
    SDL_SetTextureAlphaMod(im.atlas.sdl(), 255);
    if (style.shadow) build(style.shadow_offset, *style.shadow);
    if (style.outline) {
        const float d = style.outline_width;
        for (int dy = -1; dy <= 1; ++dy)
            for (int dx = -1; dx <= 1; ++dx)
                if (dx || dy) build({static_cast<float>(dx) * d, static_cast<float>(dy) * d}, *style.outline);
    }
    build({}, style.color);
    return measure_block(text, style);
}

Vec2 Font::draw_in_box(Renderer& renderer, std::string_view text, RectF box, const TextStyle& style) const {
    const Vec2 size = measure_block(text, style);
    float x = box.x;
    if (style.align == Align::Center) x = box.x + box.w * 0.5f;
    if (style.align == Align::Right) x = box.right();
    float y = box.y + (box.h - size.y) * 0.5f;
    if (impl_ && !impl_->smooth) y = std::round(y);
    return draw(renderer, text, {x, y}, style);
}

}  // namespace vs
