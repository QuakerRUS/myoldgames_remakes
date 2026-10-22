#include "keyboard_view.hpp"

#include <array>
#include <cmath>

#include "draw_util.hpp"
#include "palette.hpp"

namespace ks {

namespace {
constexpr float kGap = 6.0f;      // between keys
constexpr float kDepth = 4.0f;    // visible key side under the face
constexpr float kRadius = 9.0f;
constexpr float kWrongFlashMs = 450.0f;
constexpr float kPressSeconds = 0.16f;
}  // namespace

KeyboardView::KeyboardView(vs::RectF keys_area, vs::RectF legend_area, const kbsim::Layout& layout)
    : layout_(&layout), area_(keys_area), legend_(legend_area) {
    unit_ = keys_area.w / layout.width();
    const auto& keys = layout.keys();
    rects_.reserve(keys.size());
    for (const kbsim::Key& k : keys) {
        rects_.push_back({area_.x + k.x * unit_ + kGap * 0.5f, area_.y + static_cast<float>(k.row) * unit_ + kGap * 0.5f,
                          k.width * unit_ - kGap, unit_ - kGap});
    }
    press_.assign(keys.size(), 0.0f);
}

void KeyboardView::reset() { std::fill(press_.begin(), press_.end(), 0.0f); }

void KeyboardView::on_correct(kbsim::KeyIndex key, kbsim::KeyIndex shift_key) {
    if (key >= 0 && static_cast<std::size_t>(key) < press_.size()) press_[static_cast<std::size_t>(key)] = 1.0f;
    if (shift_key >= 0 && static_cast<std::size_t>(shift_key) < press_.size())
        press_[static_cast<std::size_t>(shift_key)] = 1.0f;
}

void KeyboardView::update(double dt) {
    for (float& p : press_) p = std::max(0.0f, p - static_cast<float>(dt) / kPressSeconds);
}

void KeyboardView::draw_label(vs::Renderer& r, const Fonts& f, const kbsim::Key& key, vs::RectF face,
                              vs::Color ink) const {
    vs::TextStyle st;
    st.color = ink;
    st.align = vs::Align::Center;
    if (key.normal == U' ') return;  // the space bar has no caption
    if (key.normal == 0) {
        // Modifiers: a small caption.
        st.color = fade(ink, 0.62f);
        f.key_small.draw_in_box(r, key.label, face, st);
        return;
    }
    if (!key.label.empty()) {
        f.key.draw_in_box(r, key.label, offset(face, 0, -1), st);  // letter keys: the capital letter
        return;
    }
    // Symbol keys: the shifted character small at the top left, the unshifted one larger.
    if (key.shifted) {
        vs::TextStyle sm;
        sm.color = fade(ink, 0.6f);
        f.key_small.draw(r, to_utf8(key.shifted), {face.x + 8.0f, face.y + 4.0f}, sm);
    }
    const vs::RectF lower{face.x + face.w * 0.22f, face.y + face.h * 0.28f, face.w * 0.7f, face.h * 0.68f};
    f.key.draw_in_box(r, to_utf8(key.normal), lower, st);
}

void KeyboardView::draw_key(vs::Renderer& r, const Fonts& f, kbsim::KeyIndex k, const Hint& h) const {
    const kbsim::Key& key = layout_->key(k);
    const pal::FingerStyle& fs = pal::finger_style(key.finger);
    const vs::RectF slot = rects_[static_cast<std::size_t>(k)];
    const float p = ease_out(press_[static_cast<std::size_t>(k)]);
    const bool target = k == h.key || k == h.shift;
    const float wrong = k == h.wrong ? h.wrong_t : 0.0f;

    if (target) draw_glow(r, slot, kRadius, fade(pal::accent, 0.30f + 0.25f * h.pulse), 7.0f + 2.0f * h.pulse);

    // Side (the darker base), then the face, which sinks while pressed.
    r.fill_rounded_rect({slot.x, slot.y + kDepth, slot.w, slot.h - kDepth}, kRadius, fs.edge);
    const vs::RectF face{slot.x, slot.y + p * (kDepth - 1.0f), slot.w, slot.h - kDepth};
    vs::Color top = mix(fs.face, vs::rgb(0xFFFFFF), 0.45f);
    vs::Color bottom = fs.face;
    if (target) {
        top = mix(top, vs::rgb(0xFFFFFF), 0.5f);
        bottom = mix(bottom, vs::rgb(0xFFFFFF), 0.35f);
    }
    r.fill_gradient(face, top, bottom, kRadius);
    if (wrong > 0) r.fill_rounded_rect(face, kRadius, fade(pal::error, 0.55f * wrong));
    if (target) r.draw_rounded_rect(face, kRadius, pal::accent, 2.5f);
    else if (wrong > 0) r.draw_rounded_rect(face, kRadius, fade(pal::error, wrong), 2.5f);
    else r.draw_rounded_rect(face, kRadius, fade(fs.edge, 0.55f), 1.0f);

    if (key.bump) {
        // Tactile bump of the home keys А and О.
        r.fill_rounded_rect({face.center().x - 8.0f, face.bottom() - 9.0f, 16.0f, 3.0f}, 1.5f, fade(pal::key_ink, 0.45f));
    }
    vs::Color ink = target ? pal::accent_dark : pal::key_ink;
    if (wrong > 0) ink = mix(ink, vs::rgb(0xB4232A), wrong);
    draw_label(r, f, key, face, ink);
}

void KeyboardView::draw_legend(vs::Renderer& r, const Fonts& f, const kbsim::Session& s, const Hint& h) const {
    using kbsim::Finger;
    const Finger active = s.expected().finger;
    const bool show = h.key != kbsim::kNoKey;
    std::optional<Finger> shift_finger;
    if (h.shift != kbsim::kNoKey) shift_finger = layout_->key(h.shift).finger;

    constexpr float kPillW = 46.0f, kPillH = 36.0f, kSpace = 8.0f;
    const std::array<Finger, 5> left = {Finger::LeftPinky, Finger::LeftRing, Finger::LeftMiddle, Finger::LeftIndex,
                                        Finger::Thumbs};
    const std::array<Finger, 5> right = {Finger::Thumbs, Finger::RightIndex, Finger::RightMiddle, Finger::RightRing,
                                         Finger::RightPinky};
    const float group_w = 5 * kPillW + 4 * kSpace;
    const float y = legend_.y + (legend_.h - kPillH) * 0.5f;

    auto pill = [&](Finger fg, float x) {
        const pal::FingerStyle& fs = pal::finger_style(fg);
        const bool on = show && fg == active;
        const bool on_shift = show && shift_finger && *shift_finger == fg;
        vs::RectF rc{x, y - (on ? 3.0f : 0.0f), kPillW, kPillH};
        if (on) draw_glow(r, rc, 10.0f, fade(pal::accent, 0.30f + 0.25f * h.pulse), 7.0f);
        r.fill_rounded_rect(offset(rc, 0, 3), 10.0f, fs.edge);
        r.fill_gradient(rc, mix(fs.face, vs::rgb(0xFFFFFF), 0.4f), fs.face, 10.0f);
        if (on) r.draw_rounded_rect(rc, 10.0f, pal::accent, 2.5f);
        else if (on_shift) r.draw_rounded_rect(rc, 10.0f, fade(pal::accent, 0.7f), 1.5f);
        vs::TextStyle st;
        st.color = on ? pal::accent_dark : fs.accent;
        st.align = vs::Align::Center;
        f.ui_bold.draw_in_box(r, std::to_string(kbsim::legend_number(fg)), offset(rc, 0, -1), st);
    };
    for (std::size_t i = 0; i < left.size(); ++i) pill(left[i], legend_.x + static_cast<float>(i) * (kPillW + kSpace));
    const float rx = legend_.right() - group_w;
    for (std::size_t i = 0; i < right.size(); ++i) pill(right[i], rx + static_cast<float>(i) * (kPillW + kSpace));

    vs::TextStyle cap;
    cap.color = pal::muted;
    const float cy = legend_.y + (legend_.h - f.small.line_height()) * 0.5f;
    f.small.draw(r, "левая рука", {legend_.x + group_w + 16.0f, cy}, cap);
    cap.align = vs::Align::Right;
    f.small.draw(r, "правая рука", {rx - 16.0f, cy}, cap);
}

void KeyboardView::render(vs::Renderer& r, const Fonts& f, const kbsim::Session& s, std::int64_t now_ms,
                          double time_s) const {
    Hint h;
    if (!s.paused()) {
        const kbsim::Expected e = s.expected();
        h.key = e.key;
        h.shift = e.shift_key;
        if (const auto& w = s.last_wrong(); w && w->key != kbsim::kNoKey) {
            h.wrong = w->key;
            h.wrong_t = clamp01(1.0f - static_cast<float>(now_ms - w->at_ms) / kWrongFlashMs);
        }
    }
    h.pulse = 0.5f + 0.5f * static_cast<float>(std::sin(time_s * 4.0));
    for (kbsim::KeyIndex k = 0; k < static_cast<kbsim::KeyIndex>(rects_.size()); ++k)
        if (k != h.key && k != h.shift) draw_key(r, f, k, h);
    // Highlighted keys last, so their glow lies over the neighbours.
    if (h.shift != kbsim::kNoKey) draw_key(r, f, h.shift, h);
    if (h.key != kbsim::kNoKey) draw_key(r, f, h.key, h);
    draw_legend(r, f, s, h);
}

}  // namespace ks
