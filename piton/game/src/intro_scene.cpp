// Intro: the animated "V Software Entertainment ... presents" logo, then the PITON title with
// the credits. Any key (or click) skips to the menu.
#include <SDL3/SDL_log.h>

#include <algorithm>
#include <exception>
#include <filesystem>

#include <vs/engine/app.hpp>
#include <vs/engine/storage.hpp>
#include <vs/engine/texture.hpp>

#include "easing.hpp"
#include "launch_options.hpp"
#include "palette.hpp"
#include "scenes.hpp"
#include "screenshot.hpp"
#include "ui_draw.hpp"

namespace piton_game {
namespace {

constexpr double kTitleStart = 5.4;   // the PITON part begins
constexpr double kEnd = kTitleStart + 6.4;

std::filesystem::path branding(const char* name) {
    const std::string rel = std::string("branding/") + name;
    std::filesystem::path p = vs::find_asset(rel);
    if (!p.empty()) return p;
#ifdef VS_BRANDING_DIR
    return vs::utf8_to_path(VS_BRANDING_DIR) / name;
#else
    return vs::asset_path(rel);
#endif
}

vs::Color alpha_of(vs::Color c, float a) { return c.with_alpha(static_cast<std::uint8_t>(ease::clamp01(a) * c.a)); }

class IntroScene : public vs::Scene {
public:
    explicit IntroScene(SharedPtr shared) : shared_(std::move(shared)) {}

    void on_enter() override {
        shared_ = Shared::ensure(shared_, app());
        vs::Renderer& r = app().renderer();
        const auto smooth = vs::ScaleMode::Linear;
        emblem_ = vs::Texture::load(r, branding("vse_emblem.png"), smooth);
        software_ = vs::Texture::load(r, branding("vse_word_software.png"), smooth);
        rule_ = vs::Texture::load(r, branding("vse_rule.png"), smooth);
        entertainment_ = vs::Texture::load(r, branding("vse_word_entertainment.png"), smooth);
        presents_ = vs::Texture::load(r, branding("presents.png"), smooth);
        if (launch_options().shot.active()) time_ = launch_options().shot.time;
    }

    void update(double dt) override {
        if (shot_.active()) return;
        const double before = time_;
        time_ += dt;
        if (before < kTitleStart && time_ >= kTitleStart) shared_->music(Music::Menu);
        if (app().input().any_pressed() || time_ >= kEnd) finish();
    }

    void render(vs::Renderer& r, double) override {
        if (time_ < kTitleStart)
            render_company(r);
        else
            render_title(r, time_ - kTitleStart);
        shot_.capture(app(), r);
    }

private:
    void finish() {
        if (done_) return;
        done_ = true;
        if (time_ < kTitleStart) shared_->music(Music::Menu);
        app().replace_scene(make_menu_scene(shared_));
    }

    // Draws `tex` scaled by `s` with its centre at `c`.
    static void draw_centered(vs::Renderer& r, const vs::Texture& tex, vs::Vec2 c, float s, float alpha) {
        const float w = static_cast<float>(tex.width()) * s;
        const float h = static_cast<float>(tex.height()) * s;
        r.draw_stretched(tex, std::nullopt, {c.x - w * 0.5f, c.y - h * 0.5f, w, h},
                         vs::Color{255, 255, 255, static_cast<std::uint8_t>(ease::clamp01(alpha) * 255)});
    }

    void render_company(vs::Renderer& r) {
        const double t = time_;
        r.fill_gradient({0, 0, 1280, 720}, vs::rgb(0x05080F), vs::rgb(0x0C1830));

        // Emblem: zooms in from large and settles with a slight overshoot.
        const float e = ease::phase(t, 0.2, 1.0);
        const float scale = 0.46f * (1.0f + 0.9f * (1.0f - ease::out_back(e)));
        draw_centered(r, emblem_, {640, 214}, scale, ease::phase(t, 0.2, 0.6));

        // "SOFTWARE" slides in from the left.
        const float s = ease::out_cubic(ease::phase(t, 1.0, 0.6));
        draw_centered(r, software_, {640 - 80 * (1 - s), 410}, 0.62f, s);

        // The rule grows from the centre.
        const float g = ease::out_cubic(ease::phase(t, 1.4, 0.6));
        const float rw = static_cast<float>(rule_.width()) * 0.62f * g;
        if (rw > 1) r.draw_stretched(rule_, std::nullopt, {640 - rw * 0.5f, 452, rw, 3});

        // "ENTERTAINMENT" is revealed letter by letter (left to right).
        const float reveal = ease::phase(t, 1.8, 1.0);
        if (reveal > 0) {
            const float w = static_cast<float>(entertainment_.width()) * 0.62f;
            const float h = static_cast<float>(entertainment_.height()) * 0.62f;
            r.set_clip(vs::RectF{640 - w * 0.5f, 462, w * reveal, h + 2});
            r.draw_stretched(entertainment_, std::nullopt, {640 - w * 0.5f, 462, w, h});
            r.set_clip(std::nullopt);
        }

        // "presents" fades in.
        draw_centered(r, presents_, {640, 590}, 0.55f, ease::phase(t, 3.0, 0.7));

        // Fade out to black before the title.
        const float out = ease::phase(t, kTitleStart - 0.6, 0.6);
        if (out > 0) r.fill_rect({0, 0, 1280, 720}, vs::Color{0, 0, 0, static_cast<std::uint8_t>(out * 255)});
    }

    void render_title(vs::Renderer& r, double t) {
        const Assets& a = shared_->assets;
        draw_backdrop(r, a, 0.45f);

        // The logo drops in and bounces.
        const float drop = ease::out_bounce(ease::phase(t, 0.3, 1.0));
        const float lw = static_cast<float>(a.logo.width());
        const float y = -260.0f + (120.0f + 260.0f) * drop;
        r.draw(a.logo, std::nullopt, {640 - lw * 0.5f, std::round(y)});

        // Credits fade in one by one.
        auto line = [&](const char* text, float y_pos, double start, vs::Color c, const vs::Font& f) {
            const float k = ease::phase(t, start, 0.5);
            if (k <= 0) return;
            draw_text(r, f, text, {640, y_pos + 8 * (1 - k)}, alpha_of(c, k), vs::Align::Center, TextFx::Both);
        };
        line("Program by Fedoseyenko Victor", 430, 1.4, pal::kText, a.font16);
        line("Russia, Moscow, 2026", 470, 1.8, pal::kText, a.font16);
        line("Version 2.0", 540, 2.3, pal::kGold, a.font24);
        line("V Software Entertainment", 650, 2.8, pal::kTextDim, a.font8);

        // Fade in from black at the start, fade out at the end.
        const float in = 1.0f - ease::phase(t, 0.0, 0.5);
        const float out = ease::phase(t, kEnd - kTitleStart - 0.5, 0.5);
        const float black = std::max(in, out);
        if (black > 0) r.fill_rect({0, 0, 1280, 720}, vs::Color{0, 0, 0, static_cast<std::uint8_t>(black * 255)});
    }

    SharedPtr shared_;
    vs::Texture emblem_, software_, rule_, entertainment_, presents_;
    double time_ = 0;
    bool done_ = false;
    ShotTaker shot_;
};

}  // namespace

std::unique_ptr<vs::Scene> make_intro_scene(SharedPtr shared) { return std::make_unique<IntroScene>(std::move(shared)); }

}  // namespace piton_game
