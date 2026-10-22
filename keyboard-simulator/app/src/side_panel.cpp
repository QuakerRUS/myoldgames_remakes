#include "side_panel.hpp"

#include <string>

#include "draw_util.hpp"
#include "palette.hpp"

namespace ks {

namespace {
constexpr float kRadius = 12.0f;
constexpr std::array<const char*, 4> kHotkeys = {"F2", "F3", "F1", "F4"};

vs::RectF button_rect(vs::RectF area, int i) {
    constexpr float kGap = 12.0f;
    const float h = (area.h - 3 * kGap) / 4.0f;
    return {area.x, area.y + static_cast<float>(i) * (h + kGap), area.w, h};
}

void draw_row(vs::Renderer& r, const Fonts& f, float x, float y, float w, const char* label, const std::string& value) {
    vs::TextStyle l;
    l.color = pal::muted;
    f.small.draw(r, label, {x, y}, l);
    vs::TextStyle v;
    v.color = pal::ink;
    v.align = vs::Align::Right;
    f.ui_bold.draw(r, value, {x + w, y - 2.0f}, v);
}
}  // namespace

SidePanel::SidePanel(const vs::Theme* theme, vs::RectF buttons_area, vs::RectF card_area, vs::RectF sound_area,
                     Actions a)
    : buttons_{vs::Button(theme, "Рестарт", button_rect(buttons_area, 0), a.restart),
               vs::Button(theme, "Пауза", button_rect(buttons_area, 1), a.pause),
               vs::Button(theme, "Помощь", button_rect(buttons_area, 2), a.help),
               vs::Button(theme, "О программе", button_rect(buttons_area, 3), a.about)},
      sound_(theme, "Звук", sound_area, a.sound),
      card_(card_area) {
    buttons_[0].hotkey = SDLK_F2;
    buttons_[1].hotkey = SDLK_F3;
    buttons_[2].hotkey = SDLK_F1;
    buttons_[3].hotkey = SDLK_F4;
    sound_.hotkey = SDLK_F5;
}

void SidePanel::update(const vs::Input& input) {
    for (vs::Button& b : buttons_) b.update(input);
    sound_.update(input);
}

void SidePanel::draw_button(vs::Renderer& r, const Fonts& f, const vs::Button& b, std::string_view label,
                            std::string_view hotkey, bool primary) const {
    vs::RectF rc = b.rect;
    if (b.pressed()) rc = offset(rc, 0, 1.5f);
    vs::Color fill = primary ? pal::accent : pal::card;
    if (b.hovered()) fill = primary ? mix(pal::accent, pal::accent_dark, 0.35f) : mix(pal::card, pal::accent_soft, 0.55f);
    if (b.pressed()) fill = primary ? pal::accent_dark : pal::accent_soft;
    ks::draw_card(r, rc, kRadius, fill, b.pressed() ? 0.4f : 1.0f);
    if (primary) r.fill_rounded_rect(rc, kRadius, fill);  // no hairline on the filled button

    vs::TextStyle st;
    st.color = primary ? vs::rgb(0xFFFFFF) : pal::ink;
    const float ty = rc.y + (rc.h - f.ui_bold.line_height()) * 0.5f;
    f.ui_bold.draw(r, label, {rc.x + 20.0f, ty}, st);

    // Hotkey chip on the right.
    const float cw = f.small_bold.measure(hotkey) + 14.0f;
    const vs::RectF chip{rc.right() - 16.0f - cw, rc.center().y - 12.0f, cw, 24.0f};
    r.fill_rounded_rect(chip, 6.0f, primary ? vs::rgba(0xFFFFFF33) : pal::card_alt);
    if (!primary) r.draw_rounded_rect(chip, 6.0f, pal::divider, 1.0f);
    vs::TextStyle cs;
    cs.color = primary ? vs::rgb(0xFFFFFF) : pal::muted;
    cs.align = vs::Align::Center;
    f.small_bold.draw_in_box(r, hotkey, chip, cs);
}

void SidePanel::draw_next_card(vs::Renderer& r, const Fonts& f, const kbsim::Session& s) const {
    ks::draw_card(r, card_, 16.0f, pal::card);
    const float pad = 20.0f;
    const float x = card_.x + pad, w = card_.w - 2 * pad;
    float y = card_.y + 18.0f;

    vs::TextStyle cap;
    cap.color = pal::muted;
    cap.align = vs::Align::Center;
    f.small.draw(r, "Следующая клавиша", {card_.center().x, y}, cap);
    y += 30.0f;

    const kbsim::Expected e = s.expected();
    const bool paused = s.paused();
    const pal::FingerStyle& fs = pal::finger_style(e.finger);
    const float cap_w = e.ch == U' ' ? 176.0f : 112.0f;
    const vs::RectF keycap{card_.center().x - cap_w * 0.5f, y, cap_w, 104.0f};
    if (paused) {
        r.fill_rounded_rect(offset(keycap, 0, 5), 16.0f, pal::divider);
        r.fill_rounded_rect(keycap, 16.0f, pal::card_alt);
        vs::TextStyle st;
        st.color = pal::faint;
        st.align = vs::Align::Center;
        f.ui_bold.draw_in_box(r, "пауза", keycap, st);
    } else {
        r.fill_rounded_rect(offset(keycap, 0, 5), 16.0f, fs.edge);
        r.fill_gradient(keycap, mix(fs.face, vs::rgb(0xFFFFFF), 0.5f), fs.face, 16.0f);
        r.draw_rounded_rect(keycap, 16.0f, pal::accent, 2.5f);
        vs::TextStyle st;
        st.color = pal::accent_dark;
        st.align = vs::Align::Center;
        if (e.ch == U' ')
            f.title.draw_in_box(r, "пробел", keycap, st);
        else
            f.big.draw_in_box(r, to_utf8(e.ch), offset(keycap, 0, -3), st);
    }
    y = keycap.bottom() + 20.0f;

    vs::TextStyle name;
    name.color = paused ? pal::faint : pal::ink;
    name.align = vs::Align::Center;
    f.ui_bold.draw(r, paused ? std::string("—") : std::string(pal::finger_name(e.finger)), {card_.center().x, y}, name);
    y += 28.0f;
    if (!paused && e.shift_key != kbsim::kNoKey) {
        const bool right = s.layout().key(e.shift_key).code == "ShiftRight";
        vs::TextStyle sh;
        sh.color = pal::accent;
        sh.align = vs::Align::Center;
        f.small.draw(r, right ? "+ правый Shift" : "+ левый Shift", {card_.center().x, y}, sh);
    }

    // Session counters at the bottom of the card.
    const float by = card_.bottom() - 74.0f;
    r.fill_rect({x, by, w, 1.0f}, pal::divider);
    draw_row(r, f, x, by + 14.0f, w, "Предложений", std::to_string(s.sentences_completed()));
    draw_row(r, f, x, by + 42.0f, w, "Набрано знаков", std::to_string(s.typed()));
}

void SidePanel::draw_sound(vs::Renderer& r, const Fonts& f, bool on) const {
    const vs::Button& b = sound_;
    vs::RectF rc = b.pressed() ? offset(b.rect, 0, 1.5f) : b.rect;
    ks::draw_card(r, rc, kRadius, b.hovered() ? mix(pal::card, pal::accent_soft, 0.55f) : pal::card,
                  b.pressed() ? 0.4f : 1.0f);
    vs::TextStyle st;
    st.color = pal::ink;
    f.ui_bold.draw(r, "Звук", {rc.x + 20.0f, rc.y + (rc.h - f.ui_bold.line_height()) * 0.5f}, st);
    vs::TextStyle hk;
    hk.color = pal::faint;
    f.small.draw(r, "F5", {rc.x + 72.0f, rc.y + (rc.h - f.small.line_height()) * 0.5f + 1.0f}, hk);

    // Switch.
    const vs::RectF track{rc.right() - 20.0f - 46.0f, rc.center().y - 13.0f, 46.0f, 26.0f};
    r.fill_rounded_rect(track, 13.0f, on ? pal::accent : vs::rgb(0xC9CFD9));
    const float kx = on ? track.right() - 13.0f : track.x + 13.0f;
    r.fill_circle({kx, track.center().y + 1.0f}, 10.5f, vs::rgba(0x0000001E));
    r.fill_circle({kx, track.center().y}, 10.0f, vs::rgb(0xFFFFFF));
    vs::TextStyle lbl;
    lbl.color = on ? pal::accent : pal::muted;
    lbl.align = vs::Align::Right;
    f.small.draw(r, on ? "вкл" : "выкл", {track.x - 10.0f, rc.y + (rc.h - f.small.line_height()) * 0.5f}, lbl);
}

void SidePanel::render(vs::Renderer& r, const Fonts& f, const kbsim::Session& s, bool user_paused, bool sound_on,
                       double) const {
    draw_button(r, f, buttons_[0], "Рестарт", kHotkeys[0], false);
    draw_button(r, f, buttons_[1], user_paused ? "Продолжить" : "Пауза", kHotkeys[1], user_paused);
    draw_button(r, f, buttons_[2], "Помощь", kHotkeys[2], false);
    draw_button(r, f, buttons_[3], "О программе", kHotkeys[3], false);
    draw_next_card(r, f, s);
    draw_sound(r, f, sound_on);
}

}  // namespace ks
