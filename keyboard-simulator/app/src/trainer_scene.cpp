#include "trainer_scene.hpp"

#include <chrono>

#include <SDL3/SDL.h>
#include <kbsim/text_source.hpp>
#include <kbsim/utf8.hpp>
#include <vs/engine/app.hpp>
#include <vs/engine/screenshot.hpp>
#include <vs/engine/storage.hpp>

#include "dialogs.hpp"
#include "draw_util.hpp"
#include "palette.hpp"
#include "stats_bar.hpp"

namespace ks {

namespace {
// Screen layout (logical 1280x720).
constexpr vs::RectF kTextCard{24, 24, 968, 256};
constexpr vs::RectF kTextArea{52, 42, 912, 180};
constexpr vs::RectF kStatsBar{52, 228, 912, 50};
constexpr vs::RectF kKeyboardCard{24, 296, 968, 400};
constexpr vs::RectF kKeysArea{58, 318, 900, 300};
constexpr vs::RectF kLegendArea{58, 630, 900, 52};
constexpr vs::RectF kButtonsArea{1008, 24, 248, 256};
constexpr vs::RectF kNextKeyCard{1008, 296, 248, 334};
constexpr vs::RectF kSoundArea{1008, 642, 248, 54};
constexpr vs::Vec2 kScreen{1280, 720};

constexpr std::int64_t kScriptKeyMs = 190;  // simulated typing pace
constexpr std::int64_t kLayoutHintMs = 4000;

std::uint64_t time_seed() {
    return static_cast<std::uint64_t>(std::chrono::high_resolution_clock::now().time_since_epoch().count());
}

bool is_latin_letter(char32_t c) { return (c >= U'a' && c <= U'z') || (c >= U'A' && c <= U'Z'); }
}  // namespace

std::int64_t TrainerScene::now() const {
    // Scripted runs (screenshots) use the fixed-step scene time, so the result does not depend
    // on how fast the offscreen renderer is.
    const std::int64_t base = options_.scripted() ? 1'000'000 + static_cast<std::int64_t>(time_ * 1000.0)
                                                  : static_cast<std::int64_t>(SDL_GetTicks());
    return base + clock_offset_;
}

void TrainerScene::on_enter() {
    vs::Renderer& r = app().renderer();
    settings_ = Settings::load();
    fonts_ = Fonts::load(r);
    sounds_.load();
    sounds_.enabled = settings_.sound;
    try {
        icon_ = vs::Texture::load(r, vs::asset_path("gfx/icon.png"), vs::ScaleMode::Linear);
    } catch (...) {
    }

    theme_.font = &fonts_.ui;
    theme_.on_sound = [this](vs::UiSound) { sounds_.play(app().audio(), Sfx::Click); };

    layout_ = std::make_unique<kbsim::Layout>(kbsim::Layout::load(vs::asset_path("layout/ru.json")));
    const std::uint64_t seed = options_.seed.value_or(time_seed());
    session_ = std::make_unique<kbsim::Session>(kbsim::TextSource::load(vs::asset_path("texts/ru.txt"), seed), *layout_);

    text_view_ = std::make_unique<TextView>(kTextArea);
    const vs::Font* mono = &fonts_.mono;
    session_->wrap_measure([mono](std::u32string_view s) { return mono->measure(to_utf8(s)); },
                           text_view_->wrap_width());
    keyboard_ = std::make_unique<KeyboardView>(kKeysArea, kLegendArea, *layout_);
    panel_ = std::make_unique<SidePanel>(&theme_, kButtonsArea, kNextKeyCard, kSoundArea,
                                         SidePanel::Actions{[this] { restart(); }, [this] { toggle_pause(); },
                                                            [this] { open_modal(help_dialog([this] { resume_after_modal(); })); },
                                                            [this] { open_modal(about_dialog(&icon_, [this] { resume_after_modal(); })); },
                                                            [this] { toggle_sound(); }});
    modal_ = std::make_unique<Modal>(&fonts_, kScreen);
    modal_->on_click = [this] { sounds_.play(app().audio(), Sfx::Click); };

    app().input().start_text_input();
}

void TrainerScene::on_exit() { app().input().stop_text_input(); }

void TrainerScene::on_event(const SDL_Event& e) {
    // Losing focus pauses a running exercise (the clock should not run in the background).
    if (e.type == SDL_EVENT_WINDOW_FOCUS_LOST && !options_.scripted() && session_ && session_->started() &&
        !session_->paused())
        session_->pause(now());
}

bool TrainerScene::on_quit_request() {
    if (app().config().smoke || options_.scripted() || !modal_) return true;
    ask_quit();
    return false;
}

void TrainerScene::ask_quit() {
    if (modal_->is_open() && quit_asked_) return;  // already asking
    open_modal(quit_dialog(
        [this] {
            quit_asked_ = false;
            app().quit();
        },
        [this] {
            quit_asked_ = false;
            resume_after_modal();
        }));
    quit_asked_ = true;
}

void TrainerScene::type_char(char32_t ch, std::int64_t at_ms, bool sounds) {
    int current_lines = 0;
    for (const kbsim::Line& l : session_->lines())
        if (l.sentence == 0) ++current_lines;
    const kbsim::Expected e = session_->expected();
    const std::uint64_t done = session_->sentences_completed();
    switch (session_->type(ch, at_ms)) {
    case kbsim::TypeResult::Correct:
        keyboard_->on_correct(e.key, e.shift_key);
        if (session_->sentences_completed() != done) {
            text_view_->on_sentence_done(current_lines);
            if (sounds) sounds_.play(app().audio(), Sfx::Sentence);
        } else if (sounds) {
            sounds_.play(app().audio(), Sfx::Correct);
        }
        break;
    case kbsim::TypeResult::Wrong:
        if (sounds) sounds_.play(app().audio(), Sfx::Wrong);
        break;
    case kbsim::TypeResult::Ignored:
        break;
    }
}

void TrainerScene::restart() {
    session_->restart(now());
    modal_paused_ = false;
    text_view_->reset();
    keyboard_->reset();
}

void TrainerScene::toggle_pause() { session_->toggle_pause(now()); }

void TrainerScene::toggle_sound() {
    settings_.sound = !settings_.sound;
    sounds_.enabled = settings_.sound;
    settings_.save();
    if (settings_.sound) sounds_.play(app().audio(), Sfx::Click);
}

void TrainerScene::open_modal(Modal::Spec spec) {
    quit_asked_ = false;  // ask_quit sets it again
    if (!session_->paused()) {
        session_->pause(now());
        modal_paused_ = true;
    }
    modal_->open(std::move(spec));
}

void TrainerScene::resume_after_modal() {
    if (modal_paused_) session_->resume(now());
    modal_paused_ = false;
}

void TrainerScene::run_script_typing() {
    std::int64_t t = now();
    std::int64_t i = 0;
    auto step = [&] {
        t += kScriptKeyMs + (i++ % 3) * 40;  // a slightly uneven rhythm
    };
    for (int n = 0; n < options_.autotype; ++n) {
        type_char(session_->expected().ch, t, false);
        step();
    }
    try {
        for (char32_t ch : kbsim::utf8_to_u32(options_.type)) {
            type_char(ch, t, false);
            step();
        }
    } catch (...) {
    }
    // Move the clock forward so the typing appears to have happened just now.
    if (t > now()) clock_offset_ += t - kScriptKeyMs - now();
}

void TrainerScene::run_script_state() {
    const std::string& s = options_.state;
    if (s == "paused") session_->pause(now());
    else if (s == "help") open_modal(help_dialog([this] { resume_after_modal(); }));
    else if (s == "about") open_modal(about_dialog(&icon_, [this] { resume_after_modal(); }));
    else if (s == "quit") ask_quit();
}

void TrainerScene::update(double dt) {
    time_ += dt;
    vs::Input& in = app().input();

    // Debug script: type, open a state, take the screenshot, quit.
    if (options_.scripted()) {
        if (script_stage_ == 0 && time_ >= 0.30) {
            run_script_typing();
            run_script_state();
            script_stage_ = 1;
        } else if (script_stage_ == 1 && time_ >= 0.50) {
            script_stage_ = 2;
            shot_due_ = !options_.screenshot.empty();
        }
    }

    text_view_->update(dt, *session_, fonts_.mono);
    keyboard_->update(dt);

    if (modal_->is_open()) {
        modal_->update(in, dt);
        return;
    }
    if (in.pressed(SDLK_ESCAPE)) {
        ask_quit();
        return;
    }
    panel_->update(in);
    if (in.pressed(SDLK_PAUSE)) {
        sounds_.play(app().audio(), Sfx::Click);
        toggle_pause();
    }
    if (modal_->is_open() || in.text().empty()) return;
    try {
        for (char32_t ch : kbsim::utf8_to_u32(in.text())) type_char(ch, now(), true);
    } catch (...) {
        // Malformed input text: ignore it.
    }
}

void TrainerScene::draw_pause_badge(vs::Renderer& r) const {
    const vs::RectF badge{kTextArea.center().x - 190.0f, kTextArea.center().y - 44.0f, 380.0f, 88.0f};
    draw_card(r, badge, 16.0f, pal::card, 1.5f);
    // Two bars of the pause sign.
    const float x = badge.x + 34.0f, y = badge.center().y - 16.0f;
    r.fill_rounded_rect({x, y, 9.0f, 32.0f}, 3.0f, pal::accent);
    r.fill_rounded_rect({x + 16.0f, y, 9.0f, 32.0f}, 3.0f, pal::accent);
    vs::TextStyle t;
    t.color = pal::ink;
    fonts_.title.draw(r, "Пауза", {badge.x + 82.0f, badge.y + 12.0f}, t);
    t.color = pal::muted;
    fonts_.small.draw(r, "F3 или «Продолжить» — вернуться", {badge.x + 84.0f, badge.y + 50.0f}, t);
}

void TrainerScene::draw_layout_warning(vs::Renderer& r) const {
    const auto& w = session_->last_wrong();
    if (!w || w->key != kbsim::kNoKey || !is_latin_letter(w->typed)) return;
    const std::int64_t age = now() - w->at_ms;
    if (age > kLayoutHintMs || session_->paused()) return;
    const float a = clamp01(static_cast<float>(kLayoutHintMs - age) / 400.0f);
    const char* msg = "Включите русскую раскладку клавиатуры";
    const float wpx = fonts_.ui_bold.measure(msg) + 40.0f;
    const vs::RectF pill{kTextArea.right() - wpx, kStatsBar.y - 50.0f, wpx, 36.0f};
    draw_shadow(r, pill, 18.0f, a);
    r.fill_rounded_rect(pill, 18.0f, fade(pal::error, a));
    vs::TextStyle st;
    st.color = fade(vs::rgb(0xFFFFFF), a);
    st.align = vs::Align::Center;
    fonts_.ui_bold.draw_in_box(r, msg, pill, st);
}

void TrainerScene::draw_text_card(vs::Renderer& r) const {
    draw_card(r, kTextCard, 18.0f, pal::card);
    text_view_->render(r, *session_, fonts_, now(), time_);
    r.fill_rect({kStatsBar.x, kStatsBar.y - 2.0f, kStatsBar.w, 1.0f}, pal::divider);
    draw_stats_bar(r, fonts_, kStatsBar, *session_, now());
    if (session_->paused() && !modal_paused_) draw_pause_badge(r);
    draw_layout_warning(r);
}

void TrainerScene::render(vs::Renderer& r, double) {
    r.clear(pal::background);
    draw_text_card(r);
    draw_card(r, kKeyboardCard, 18.0f, pal::card);
    keyboard_->render(r, fonts_, *session_, now(), time_);
    panel_->render(r, fonts_, *session_, session_->paused() && !modal_paused_, settings_.sound, time_);
    modal_->render(r);

    if (shot_due_) {
        shot_due_ = false;
        vs::save_frame_png(r, vs::utf8_to_path(options_.screenshot));  // logs failures
        app().quit();
    }
}

}  // namespace ks
