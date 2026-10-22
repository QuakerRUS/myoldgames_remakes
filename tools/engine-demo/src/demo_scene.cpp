#include <SDL3/SDL.h>

#include <cmath>
#include <cstring>
#include <filesystem>
#include <numbers>
#include <vector>

#include <vs/engine/engine.hpp>

#include "demo.hpp"

namespace demo {

std::string& screenshot_path() {
    static std::string path;
    return path;
}

namespace {

namespace fs = std::filesystem;

// Writes a two-frame sprite sheet (PNG + JSON) and returns the JSON path.
fs::path write_test_sheet() {
    const fs::path dir = fs::temp_directory_path() / "vs_engine_demo";
    vs::Image img(64, 32, {0, 0, 0, 0});
    for (int y = 0; y < 32; ++y)
        for (int x = 0; x < 32; ++x) {
            const bool ring = std::abs(std::hypot(x - 15.5, y - 15.5) - 12.0) < 3.0;
            if (ring) img.set(x, y, {255, 200, 40, 255});
            if (x > 4 && x < 27 && y > 4 && y < 27) img.set(x + 32, y, {60, 180, 255, 255});
        }
    vs::save_png(dir / "demo.png", img);
    vs::write_file_atomic(dir / "demo.json", R"({
  "image": "demo.png",
  "frames": {"ring": {"x": 0, "y": 0, "w": 32, "h": 32, "pivot_x": 16, "pivot_y": 16},
             "box":  {"x": 32, "y": 0, "w": 32, "h": 32, "pivot_x": 16, "pivot_y": 16}},
  "animations": {"blink": {"loop": true, "frames": [{"frame": "ring", "ms": 300}, {"frame": "box", "ms": 300}]}}
}
)");
    return dir / "demo.json";
}

vs::Sound make_beep(double freq, double seconds) {
    const int rate = 44100;
    std::vector<std::int16_t> s(static_cast<std::size_t>(rate * seconds));
    for (std::size_t i = 0; i < s.size(); ++i) {
        const double t = static_cast<double>(i) / rate;
        const double env = 1.0 - t / seconds;
        s[i] = static_cast<std::int16_t>(std::sin(2 * std::numbers::pi * freq * t) * env * 8000);
    }
    return vs::Sound::from_samples(std::move(s), 1, rate);
}

class DemoScene : public vs::Scene {
public:
    void on_enter() override {
        if (font_.valid()) return;
        vs::Renderer& r = app().renderer();
        const fs::path font_path = "C:/Windows/Fonts/segoeui.ttf";
        font_ = vs::Font::load(r, font_path, 28.0f);
        small_ = vs::Font::load(r, font_path, 18.0f);
        if (!font_.has_glyph(U'Ж') || font_.measure("Привет") <= 0) throw vs::Error("Cyrillic glyphs missing");
        sheet_ = vs::SpriteSheet::load(r, write_test_sheet());
        player_.play(sheet_.animation("blink"));
        beep_ = make_beep(660, 0.15);
        music_ = make_beep(220, 2.0);
        app().audio().play_music(music_, true, 0.5);

        theme_.font = &font_;
        theme_.text_shadow = vs::Color{0, 0, 0, 160};
        theme_.on_sound = [this](vs::UiSound) { app().audio().play(beep_, 0.5f); };
        menu_.add("NEW GAME", [this] { status_ = "New game selected"; });
        const int speed = menu_.add("SPEED  x1");
        menu_.item(speed).on_adjust = [this, speed](int dir) {
            speed_ = std::clamp(speed_ + dir, 1, 4);
            menu_.item(speed).label = "SPEED  x" + std::to_string(speed_);
            app().audio().set_music_rate(1.0 + 0.25 * (speed_ - 1));
        };
        menu_.add("ASK", [this] {
            dialog_.open("Save changes before leaving?", vs::DialogButtons::YesNoCancel,
                         [this](vs::DialogResult) { status_ = "Dialog closed"; });
        });
        menu_.add("EXIT", [this] { app().quit(); });
        menu_.set_position({320, 260});
        field_.rect = {120, 560, 400, 48};
        field_.max_length = 8;
        field_.uppercase = true;
        field_.filter = vs::TextField::latin_upper_or_digit;
        field_.placeholder = "NAME";
        field_.on_submit = [this](const std::string& s) { status_ = "Name: " + s; };
        app().input().start_text_input();
        camera_.viewport = {700, 120, 520, 400};
        camera_.set_bounds({0, 0, 1600, 1200});
    }

    void update(double dt) override {
        vs::Input& in = app().input();
        if (dialog_.is_open()) {
            dialog_.update(in);
        } else {
            menu_.update(in);
            field_.update(in, dt);
            button_.update(in);
        }
        t_ += dt;
        prev_pos_ = pos_;
        pos_ = {800 + std::cos(static_cast<float>(t_)) * 600, 600 + std::sin(static_cast<float>(t_) * 0.7f) * 500};
        camera_.follow(pos_, dt, 4.0f);
        player_.update(dt);
    }

    void render(vs::Renderer& r, double alpha) override {
        r.fill_gradient({0, 0, 1280, 720}, vs::rgb(0x1E2A3A), vs::rgb(0x0A0C10));
        font_.draw(r, "V Software Entertainment — engine demo", {640, 30},
                   {.color = vs::rgb(0xFFE070), .align = vs::Align::Center, .outline = vs::Color{0, 0, 0, 255}});
        small_.draw(r, "Привет, мир! Съешь же ещё этих мягких французских булок, да выпей чаю.", {40, 110},
                    {.color = vs::rgb(0xD0D0D0), .max_width = 560});
        menu_.render(r);
        field_.render(r);
        button_.render(r);
        small_.draw(r, status_, {40, 680}, {.color = vs::rgb(0x90FF90)});

        // World view through the camera.
        r.set_clip(camera_.viewport);
        r.fill_rect(camera_.viewport, vs::rgb(0x203020));
        for (int y = 0; y < 1200; y += 100)
            for (int x = 0; x < 1600; x += 100)
                r.draw_rect({camera_.world_to_screen({static_cast<float>(x), static_cast<float>(y)}).x,
                             camera_.world_to_screen({static_cast<float>(x), static_cast<float>(y)}).y, 100, 100},
                            vs::rgba(0xFFFFFF30));
        const vs::Vec2 p = vs::lerp(prev_pos_, pos_, static_cast<float>(alpha));
        sheet_.draw(r, player_, camera_.world_to_screen(p), {.scale = {2, 2}, .rotation = static_cast<float>(t_ * 90)});
        r.set_clip(std::nullopt);
        r.draw_rounded_rect(camera_.viewport.inset(-3), 6, vs::rgb(0x5A6878), 3);
        r.draw_line({700, 560}, {1220, 700}, vs::rgb(0xFF6060), 3);
        r.fill_circle({1180, 600}, 30, vs::rgba(0x60A0FFC0));
        dialog_.render(r);
        if (!screenshot_path().empty() && t_ > 0.5 && !shot_taken_) {
            shot_taken_ = true;
            vs::save_frame_png(r, vs::utf8_to_path(screenshot_path()));
        }
    }

private:
    vs::Font font_, small_;
    vs::SpriteSheet sheet_;
    vs::AnimationPlayer player_;
    vs::Sound beep_, music_;
    vs::Theme theme_;
    vs::Menu menu_{&theme_};
    vs::TextField field_{&theme_};
    vs::Dialog dialog_{&theme_};
    vs::Button button_{&theme_, "Рестарт", {120, 620, 180, 44}, [this] { status_ = "Button clicked"; }};
    vs::Camera2D camera_;
    vs::Vec2 pos_{}, prev_pos_{};
    std::string status_ = "Arrows/Enter: menu; type a name; Alt+Enter: fullscreen";
    double t_ = 0;
    int speed_ = 1;
    bool shot_taken_ = false;
};

}  // namespace

std::unique_ptr<vs::Scene> make_demo_scene() { return std::make_unique<DemoScene>(); }

}  // namespace demo
