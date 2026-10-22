#include "intro_scene.hpp"

#include <algorithm>
#include <cmath>

#include "levels.hpp"
#include "ui_kit.hpp"

namespace sg {

namespace {

// Timeline (seconds).
constexpr double kCompanyEnd = 5.9;   // the company logo has faded out
constexpr double kTitleStart = 6.1;   // the SOLDIER logo slams in
constexpr double kTitleEnd = 13.5;    // automatic switch to the menu

// Layout of the stacked company logo (vse_logo_stacked.png is 1024x629; the layers below
// are its parts at 1:1) scaled by kS and placed with its top at kY0.
constexpr float kS = 0.68f;
constexpr float kY0 = 52.0f;

float clamp01(double v) { return static_cast<float>(std::clamp(v, 0.0, 1.0)); }
float ease_out(float t) { return 1 - (1 - t) * (1 - t) * (1 - t); }
float ease_out_back(float t) {
    const float c1 = 1.70158f, c3 = c1 + 1;
    return 1 + c3 * std::pow(t - 1, 3.0f) + c1 * std::pow(t - 1, 2.0f);
}
uint8_t a8(float a) { return static_cast<uint8_t>(255 * std::clamp(a, 0.0f, 1.0f)); }

// Draws `tex` with its centre at `c` (layout scale times `scale`).
void draw_centered(vs::Renderer& r, const vs::Texture& tex, vs::Vec2 c, float scale, float alpha) {
    if (!tex.valid() || alpha <= 0) return;
    vs::DrawParams p;
    p.scale = {kS * scale, kS * scale};
    p.origin = vs::Vec2{static_cast<float>(tex.width()) / 2, static_cast<float>(tex.height()) / 2};
    p.tint = vs::Color{255, 255, 255, a8(alpha)};
    r.draw(tex, std::nullopt, c, p);
}

// Layout point (pixels of the 1024x629 stacked logo) -> screen.
vs::Vec2 L(float x, float y) { return {640 + (x - 512) * kS, kY0 + y * kS}; }

}  // namespace

IntroScene::IntroScene(ResourcesPtr r) : BaseScene(std::move(r)) {
    emblem_ = res().branding("vse_emblem.png");
    software_ = res().branding("vse_word_software.png");
    rule_ = res().branding("vse_rule.png");
    entertainment_ = res().branding("vse_word_entertainment.png");
    presents_ = res().branding("presents.png");
}

void IntroScene::on_enter() { update_canvas_scaling(app()); }

void IntroScene::leave() {
    if (leaving_) return;
    leaving_ = true;
    app().replace_scene(make_menu_scene(shared_res()));
}

void IntroScene::tick(double) {
    if (time() > 0.2 && (!keys().empty() || input().mouse_pressed(SDL_BUTTON_LEFT))) leave();
    if (time() >= kTitleEnd) leave();
}

void IntroScene::draw_company(vs::Renderer& r, double t) {
    r.fill_gradient({0, 0, 1280, 720}, vs::rgb(0x050A16), vs::rgb(0x000000));
    const float out = 1.0f - clamp01((t - 5.2) / 0.7);  // global fade-out

    // Emblem: grows in with a soft pulsing glow and a flare.
    const float e = clamp01((t - 0.3) / 0.9);
    const vs::Vec2 ec = L(512, 215);
    if (e > 0) {
        const float s = 0.7f + 0.3f * ease_out_back(e);
        const float pulse = 0.5f + 0.5f * static_cast<float>(std::sin(t * 2.4));
        for (int i = 3; i >= 1; --i)
            r.fill_circle(ec, 120.0f * s + static_cast<float>(i) * 22.0f,
                          vs::Color{60, 150, 255, a8(e * out * (0.035f + 0.02f * pulse))});
        draw_centered(r, emblem_, ec, s, e * out);
    }
    const float flare = clamp01((t - 1.0) / 0.6);
    if (flare > 0 && flare < 1) {
        const float a = (1 - flare) * 0.35f * out;
        r.fill_circle(ec, 40 + 260 * ease_out(flare), vs::Color{120, 200, 255, a8(a * 0.5f)});
        r.fill_circle(ec, 20 + 120 * ease_out(flare), vs::Color{200, 235, 255, a8(a)});
    }

    // "SOFTWARE" rises into place.
    const float sw = ease_out(clamp01((t - 1.1) / 0.6));
    const vs::Vec2 swc = L(512, 477);
    draw_centered(r, software_, {swc.x, swc.y + (1 - sw) * 24}, 1.0f, sw * out);
    // A light sweep across the word.
    const float sweep = clamp01((t - 2.5) / 0.7);
    if (sweep > 0 && sweep < 1 && software_.valid()) {
        const float w = static_cast<float>(software_.width()) * kS, h = static_cast<float>(software_.height()) * kS;
        const vs::RectF box{swc.x - w / 2, swc.y - h / 2, w, h};
        r.set_clip(box);
        const float x = box.x - 80 + (box.w + 160) * sweep;
        for (int i = 0; i < 6; ++i)
            r.fill_rect({x + static_cast<float>(i) * 8 - 24, box.y, 8, box.h},
                        vs::Color{255, 255, 255, a8(0.10f * out * (1 - std::abs(static_cast<float>(i) - 2.5f) / 3))});
        r.set_clip(std::nullopt);
    }

    // The rule grows from the centre.
    const float rl = ease_out(clamp01((t - 1.6) / 0.6));
    if (rule_.valid() && rl > 0) {
        const float fw = static_cast<float>(rule_.width()), fh = static_cast<float>(rule_.height());
        const float sw2 = fw * rl;
        const vs::Vec2 rc = L(512, 530);
        r.draw_stretched(rule_, vs::RectF{(fw - sw2) / 2, 0, sw2, fh},
                         {rc.x - sw2 * kS / 2, rc.y - fh * kS / 2, sw2 * kS, fh * kS}, vs::Color{255, 255, 255, a8(out)});
    }

    // "ENTERTAINMENT" fades in with its letters drawing together.
    const float en = ease_out(clamp01((t - 2.0) / 0.8));
    draw_centered(r, entertainment_, L(512, 566), 1.0f + 0.12f * (1 - en), en * out);

    // "presents".
    const float pr = clamp01((t - 3.1) / 0.7);
    draw_centered(r, presents_, L(512, 690), 1.0f, pr * out);
}

void IntroScene::draw_title(vs::Renderer& r, double t) {
    const double lt = t - kTitleStart;
    const float in = clamp01(lt / 0.8);
    const float out = 1.0f - clamp01((t - (kTitleEnd - 0.6)) / 0.6);
    r.clear(vs::rgb(0x000000));
    r.draw(res().menu_bg, std::nullopt, {0, 0}, [&] {
        vs::DrawParams p;
        p.tint = vs::Color{255, 255, 255, a8(in * out * 0.55f)};
        return p;
    }());
    r.fill_gradient({0, 0, 1280, 720}, vs::rgba(0x00000060), vs::rgba(0x000000C0));

    // The logo slams down and shakes the screen.
    const float slam = clamp01(lt / 0.35);
    const float impact = clamp01((lt - 0.35) / 0.4);
    float shake = 0;
    if (lt > 0.35 && lt < 0.75) shake = static_cast<float>(std::sin(lt * 90.0)) * 6.0f * (1 - impact);
    if (slam > 0) {
        const float s = 1.0f + 0.9f * (1 - slam * slam);
        const float w = 1050.0f * s;
        const float sc = w / static_cast<float>(res().logo.width());
        vs::DrawParams p;
        p.scale = {sc, sc};
        p.origin = vs::Vec2{static_cast<float>(res().logo.width()) / 2, static_cast<float>(res().logo.height()) / 2};
        p.tint = vs::Color{255, 255, 255, a8(slam * out)};
        // Shadow, then the logo.
        vs::DrawParams sp = p;
        sp.tint = vs::Color{0, 0, 0, a8(slam * out * 0.6f)};
        r.draw(res().logo, std::nullopt, {640 + 10, 210 + 14 + shake}, sp);
        r.draw(res().logo, std::nullopt, {640, 210 + shake}, p);
    }
    if (lt > 0.35 && lt < 0.8) r.fill_rect({0, 0, 1280, 720}, vs::Color{255, 255, 240, a8(0.45f * (1 - impact))});

    // Credits.
    auto line = [&](const vs::Font& f, std::string_view s, float y, double start, vs::Color c) {
        const float a = clamp01((lt - start) / 0.5) * out;
        if (a <= 0) return;
        draw_text(r, f, s, {640, y + (1 - a) * 8}, c.with_alpha(a8(a)), vs::Align::Center);
    };
    line(res().font24, "Program by Fedoseyenko Victor", 400, 1.0, colors::text);
    line(res().font16, "V Software Entertainment", 452, 1.3, colors::cyan);
    line(res().font16, "Russia, Moscow, 2026", 484, 1.6, colors::dim);
    {
        const float a = clamp01((lt - 2.0) / 0.5) * out;
        if (a > 0) draw_text(r, res().font16, "Version 2.0", {1250, 684}, colors::dim.with_alpha(a8(a)), vs::Align::Right);
    }
    if (lt > 2.6) {
        const float blink = 0.55f + 0.45f * static_cast<float>(std::sin(lt * 4.0));
        draw_text(r, res().font16, "PRESS ANY KEY", {640, 610}, colors::accent.with_alpha(a8(blink * out)),
                  vs::Align::Center);
    }
}

void IntroScene::draw(vs::Renderer& r) {
    const double t = time();
    if (t < kCompanyEnd)
        draw_company(r, t);
    else if (t < kTitleStart)
        r.clear(vs::rgb(0x000000));
    else
        draw_title(r, t);
}

std::unique_ptr<vs::Scene> make_intro_scene(ResourcesPtr res) { return std::make_unique<IntroScene>(std::move(res)); }

}  // namespace sg
