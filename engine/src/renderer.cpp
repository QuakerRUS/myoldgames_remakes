#include <vs/engine/renderer.hpp>

#include <algorithm>
#include <cmath>
#include <numbers>

#include <SDL3/SDL.h>

#include <vs/engine/texture.hpp>

namespace vs {

namespace {

SDL_FColor to_fcolor(Color c) { return {c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, c.a / 255.0f}; }

SDL_FRect to_frect(RectF r) { return {r.x, r.y, r.w, r.h}; }

// Untextured SDL_RenderGeometry uses the renderer's draw blend mode too, so every primitive sets it
// (the SDL default is NONE: translucent circles / rounded rects drawn first in a frame were opaque).
void set_blend(SDL_Renderer* r) { SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND); }

void set_color(SDL_Renderer* r, Color c) {
    set_blend(r);
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
}

}  // namespace

std::vector<Vec2> rounded_rect_outline(RectF rect, float radius, int segments) {
    radius = std::clamp(radius, 0.0f, std::min(rect.w, rect.h) * 0.5f);
    segments = std::max(segments, 1);
    std::vector<Vec2> pts;
    pts.reserve(static_cast<std::size_t>(segments + 1) * 4);
    // Corner centres and start angles (screen coordinates: y down, clockwise order).
    const Vec2 centers[4] = {{rect.x + radius, rect.y + radius},
                             {rect.right() - radius, rect.y + radius},
                             {rect.right() - radius, rect.bottom() - radius},
                             {rect.x + radius, rect.bottom() - radius}};
    const float pi = std::numbers::pi_v<float>;
    const float starts[4] = {pi, 1.5f * pi, 0.0f, 0.5f * pi};
    for (int c = 0; c < 4; ++c) {
        for (int i = 0; i <= segments; ++i) {
            const float a = starts[c] + 0.5f * pi * static_cast<float>(i) / static_cast<float>(segments);
            pts.push_back({centers[c].x + std::cos(a) * radius, centers[c].y + std::sin(a) * radius});
        }
    }
    return pts;
}

Vec2 Renderer::size() const {
    int w = 0;
    int h = 0;
    SDL_RendererLogicalPresentation mode{};
    SDL_GetRenderLogicalPresentation(renderer_, &w, &h, &mode);
    if (mode == SDL_LOGICAL_PRESENTATION_DISABLED || w == 0) SDL_GetCurrentRenderOutputSize(renderer_, &w, &h);
    return {static_cast<float>(w), static_cast<float>(h)};
}

void Renderer::clear(Color color) {
    SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
    SDL_RenderClear(renderer_);
}

void Renderer::fill_rect(RectF rect, Color color) {
    set_color(renderer_, color);
    const SDL_FRect r = to_frect(rect);
    SDL_RenderFillRect(renderer_, &r);
}

void Renderer::draw_rect(RectF rect, Color color, float thickness) {
    if (thickness <= 1.0f) {
        set_color(renderer_, color);
        const SDL_FRect r = to_frect(rect);
        SDL_RenderRect(renderer_, &r);
        return;
    }
    const float t = std::min(thickness, std::min(rect.w, rect.h) * 0.5f);
    fill_rect({rect.x, rect.y, rect.w, t}, color);
    fill_rect({rect.x, rect.bottom() - t, rect.w, t}, color);
    fill_rect({rect.x, rect.y + t, t, rect.h - 2 * t}, color);
    fill_rect({rect.right() - t, rect.y + t, t, rect.h - 2 * t}, color);
}

void Renderer::draw_line(Vec2 a, Vec2 b, Color color, float thickness) {
    if (thickness <= 1.0f) {
        set_color(renderer_, color);
        SDL_RenderLine(renderer_, a.x, a.y, b.x, b.y);
        return;
    }
    const Vec2 d = b - a;
    const float len = std::sqrt(d.x * d.x + d.y * d.y);
    if (len <= 0.0f) return;
    const Vec2 n = Vec2{-d.y / len, d.x / len} * (thickness * 0.5f);
    fill_polygon_fan({a + n, b + n, b - n, a - n}, color);
}

void Renderer::fill_polygon_fan(const std::vector<Vec2>& points, Color color) {
    if (points.size() < 3) return;
    const SDL_FColor fc = to_fcolor(color);
    std::vector<SDL_Vertex> verts;
    verts.reserve(points.size());
    for (const Vec2& p : points) verts.push_back({{p.x, p.y}, fc, {0, 0}});
    std::vector<int> indices;
    indices.reserve((points.size() - 2) * 3);
    for (int i = 1; i + 1 < static_cast<int>(points.size()); ++i) {
        indices.push_back(0);
        indices.push_back(i);
        indices.push_back(i + 1);
    }
    set_blend(renderer_);
    SDL_RenderGeometry(renderer_, nullptr, verts.data(), static_cast<int>(verts.size()), indices.data(),
                       static_cast<int>(indices.size()));
}

void Renderer::fill_rounded_rect(RectF rect, float radius, Color color) {
    if (radius <= 0.5f) {
        fill_rect(rect, color);
        return;
    }
    fill_polygon_fan(rounded_rect_outline(rect, radius), color);
}

void Renderer::draw_rounded_rect(RectF rect, float radius, Color color, float thickness) {
    const float t = std::max(thickness, 1.0f);
    const std::vector<Vec2> outer = rounded_rect_outline(rect, radius);
    const std::vector<Vec2> inner = rounded_rect_outline(rect.inset(t), std::max(radius - t, 0.0f));
    const SDL_FColor fc = to_fcolor(color);
    std::vector<SDL_Vertex> verts;
    verts.reserve(outer.size() * 2);
    for (std::size_t i = 0; i < outer.size(); ++i) {
        verts.push_back({{outer[i].x, outer[i].y}, fc, {0, 0}});
        verts.push_back({{inner[i].x, inner[i].y}, fc, {0, 0}});
    }
    std::vector<int> indices;
    const int n = static_cast<int>(outer.size());
    for (int i = 0; i < n; ++i) {
        const int j = (i + 1) % n;
        const int o0 = 2 * i, i0 = 2 * i + 1, o1 = 2 * j, i1 = 2 * j + 1;
        indices.insert(indices.end(), {o0, o1, i0, i0, o1, i1});
    }
    set_blend(renderer_);
    SDL_RenderGeometry(renderer_, nullptr, verts.data(), static_cast<int>(verts.size()), indices.data(),
                       static_cast<int>(indices.size()));
}

void Renderer::fill_circle(Vec2 center, float radius, Color color) {
    const int segments = std::clamp(static_cast<int>(radius), 12, 96);
    std::vector<Vec2> pts;
    pts.reserve(static_cast<std::size_t>(segments));
    for (int i = 0; i < segments; ++i) {
        const float a = 2.0f * std::numbers::pi_v<float> * static_cast<float>(i) / static_cast<float>(segments);
        pts.push_back({center.x + std::cos(a) * radius, center.y + std::sin(a) * radius});
    }
    fill_polygon_fan(pts, color);
}

void Renderer::fill_gradient(RectF rect, Color top, Color bottom, float radius) {
    const std::vector<Vec2> pts = radius > 0.5f ? rounded_rect_outline(rect, radius)
                                                : std::vector<Vec2>{{rect.x, rect.y},
                                                                    {rect.right(), rect.y},
                                                                    {rect.right(), rect.bottom()},
                                                                    {rect.x, rect.bottom()}};
    const SDL_FColor ct = to_fcolor(top);
    const SDL_FColor cb = to_fcolor(bottom);
    std::vector<SDL_Vertex> verts;
    verts.reserve(pts.size());
    for (const Vec2& p : pts) {
        const float t = rect.h > 0 ? std::clamp((p.y - rect.y) / rect.h, 0.0f, 1.0f) : 0.0f;
        const SDL_FColor c{ct.r + (cb.r - ct.r) * t, ct.g + (cb.g - ct.g) * t, ct.b + (cb.b - ct.b) * t,
                           ct.a + (cb.a - ct.a) * t};
        verts.push_back({{p.x, p.y}, c, {0, 0}});
    }
    std::vector<int> indices;
    for (int i = 1; i + 1 < static_cast<int>(pts.size()); ++i) indices.insert(indices.end(), {0, i, i + 1});
    set_blend(renderer_);
    SDL_RenderGeometry(renderer_, nullptr, verts.data(), static_cast<int>(verts.size()), indices.data(),
                       static_cast<int>(indices.size()));
}

void Renderer::draw(const Texture& texture, std::optional<RectF> src, Vec2 pos, const DrawParams& params) {
    if (!texture.valid()) return;
    const RectF s = src.value_or(RectF{0, 0, static_cast<float>(texture.width()), static_cast<float>(texture.height())});
    const Vec2 origin = params.origin.value_or(Vec2{});
    const SDL_FRect sr = to_frect(s);
    const SDL_FRect dr{pos.x - origin.x * params.scale.x, pos.y - origin.y * params.scale.y, s.w * params.scale.x,
                       s.h * params.scale.y};
    SDL_SetTextureColorMod(texture.sdl(), params.tint.r, params.tint.g, params.tint.b);
    SDL_SetTextureAlphaMod(texture.sdl(), params.tint.a);
    if (params.rotation == 0.0f && params.flip == Flip::None) {
        SDL_RenderTexture(renderer_, texture.sdl(), &sr, &dr);
    } else {
        const SDL_FPoint center{origin.x * params.scale.x, origin.y * params.scale.y};
        SDL_RenderTextureRotated(renderer_, texture.sdl(), &sr, &dr, params.rotation, &center,
                                 static_cast<SDL_FlipMode>(static_cast<int>(params.flip)));
    }
}

void Renderer::draw_stretched(const Texture& texture, std::optional<RectF> src, RectF dst, Color tint) {
    if (!texture.valid()) return;
    SDL_SetTextureColorMod(texture.sdl(), tint.r, tint.g, tint.b);
    SDL_SetTextureAlphaMod(texture.sdl(), tint.a);
    const SDL_FRect dr = to_frect(dst);
    if (src) {
        const SDL_FRect sr = to_frect(*src);
        SDL_RenderTexture(renderer_, texture.sdl(), &sr, &dr);
    } else {
        SDL_RenderTexture(renderer_, texture.sdl(), nullptr, &dr);
    }
}

void Renderer::set_clip(std::optional<RectF> rect) {
    if (!rect) {
        SDL_SetRenderClipRect(renderer_, nullptr);
        return;
    }
    const SDL_Rect r{static_cast<int>(std::floor(rect->x)), static_cast<int>(std::floor(rect->y)),
                     static_cast<int>(std::ceil(rect->w)), static_cast<int>(std::ceil(rect->h))};
    SDL_SetRenderClipRect(renderer_, &r);
}

}  // namespace vs
