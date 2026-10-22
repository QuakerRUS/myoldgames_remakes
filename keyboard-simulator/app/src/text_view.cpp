#include "text_view.hpp"

#include <algorithm>
#include <cmath>

#include "draw_util.hpp"
#include "palette.hpp"

namespace ks {

namespace {
constexpr float kWrongFlashMs = 420.0f;

// Byte offset of the character `index` in the UTF-8 form of `text`.
std::size_t byte_offset(std::u32string_view text, std::size_t index) {
    return to_utf8(text.substr(0, index)).size();
}
}  // namespace

void TextView::reset() {
    scroll_ = 0;
    cursor_valid_ = false;
}

void TextView::on_sentence_done(int lines_scrolled) {
    scroll_ += static_cast<float>(lines_scrolled) * line_h_ + sentence_gap_;
}

float TextView::line_y(const kbsim::Line& line, std::size_t index) const {
    return area_.y + static_cast<float>(index) * line_h_ + (line.sentence == 1 ? sentence_gap_ : 0.0f);
}

vs::RectF TextView::cursor_target(const kbsim::Session& s, const vs::Font& mono) const {
    const auto& lines = s.lines();
    const int li = s.cursor_line();
    if (li < 0 || static_cast<std::size_t>(li) >= lines.size()) return {area_.x, area_.y, 10, line_h_};
    const kbsim::Line& line = lines[static_cast<std::size_t>(li)];
    const std::string text = to_utf8(line.text);
    const std::size_t col = s.cursor_column();
    const float x0 = mono.offset_of(text, byte_offset(line.text, col));
    float w = 0;
    if (col < line.text.size())
        w = mono.offset_of(text, byte_offset(line.text, col + 1)) - x0;
    else
        w = mono.measure(" ");  // the break space at the end of the line
    const float y = line_y(line, static_cast<std::size_t>(li));
    return {area_.x + x0 - 2.0f, y + 1.0f, w + 4.0f, line_h_ - 2.0f};
}

void TextView::update(double dt, const kbsim::Session& s, const vs::Font& mono) {
    scroll_ = approach(scroll_, 0.0f, 11.0f, dt);
    if (std::abs(scroll_) < 0.25f) scroll_ = 0;
    const vs::RectF target = cursor_target(s, mono);
    if (!cursor_valid_ || std::abs(target.y - cursor_.y) > line_h_ * 0.5f) {
        cursor_ = target;  // a new line: jump (the scroll animation carries the motion)
        cursor_valid_ = true;
        return;
    }
    cursor_.x = approach(cursor_.x, target.x, 40.0f, dt);
    cursor_.w = approach(cursor_.w, target.w, 40.0f, dt);
    cursor_.y = target.y;
    cursor_.h = target.h;
}

void TextView::draw_line(vs::Renderer& r, const kbsim::Session& s, const kbsim::Line& line, float y,
                         const vs::Font& mono, bool paused) const {
    const std::string text = to_utf8(line.text);
    const float ty = y + (line_h_ - mono.line_height()) * 0.5f;
    if (paused) {
        // Cheap blur: several faint offset copies instead of crisp text.
        const vs::Color c = fade(pal::faint, 0.13f);
        const vs::Vec2 offsets[] = {{-1.8f, 0}, {1.8f, 0}, {0, -1.8f}, {0, 1.8f}, {-1.2f, -1.2f}, {1.2f, 1.2f}};
        for (const vs::Vec2 d : offsets) {
            vs::TextStyle st;
            st.color = c;
            mono.draw(r, text, {area_.x + d.x, ty + d.y}, st);
        }
        return;
    }
    auto draw_text = [&](std::string_view part, float x, vs::Color c) {
        vs::TextStyle st;
        st.color = c;
        mono.draw(r, part, {x, ty}, st);
    };
    if (line.sentence != 0) {
        draw_text(text, area_.x, pal::faint);
        return;
    }
    // Split the line into typed | the cursor letter | the rest.
    const std::size_t cur = s.cursor();
    const std::size_t n = line.text.size();
    const std::size_t typed_end = cur <= line.begin ? 0 : std::min(cur - line.begin, n);
    auto draw_part = [&](std::size_t from, std::size_t to, vs::Color c) {
        if (from >= to) return;
        const std::size_t b0 = byte_offset(line.text, from);
        const std::size_t b1 = byte_offset(line.text, to);
        draw_text(std::string_view(text).substr(b0, b1 - b0), area_.x + mono.offset_of(text, b0), c);
    };
    draw_part(0, typed_end, pal::typed);
    if (cur >= line.begin && cur < line.begin + n) {
        draw_part(typed_end, typed_end + 1, pal::accent_dark);
        draw_part(typed_end + 1, n, pal::ink);
    } else {
        draw_part(typed_end, n, pal::ink);
    }
}

void TextView::render(vs::Renderer& r, const kbsim::Session& s, const Fonts& f, std::int64_t now_ms,
                      double time_s) const {
    const bool paused = s.paused();
    r.set_clip(vs::RectF{area_.x - 8, area_.y - 2, area_.w + 16, area_.h + 4});

    // The cursor box, under the text.
    if (!paused && cursor_valid_) {
        vs::RectF box = offset(cursor_, 0, scroll_);
        float wrong = 0;
        if (const auto& w = s.last_wrong()) wrong = 1.0f - static_cast<float>(now_ms - w->at_ms) / kWrongFlashMs;
        wrong = clamp01(wrong);
        if (wrong > 0) box.x += std::sin(static_cast<float>(now_ms - s.last_wrong()->at_ms) * 0.08f) * 3.0f * wrong;
        r.fill_rounded_rect(box, 6.0f, mix(pal::accent_soft, pal::error_soft, wrong));
        // The underline breathes gently.
        const float pulse = 0.78f + 0.22f * static_cast<float>(std::sin(time_s * 4.0));
        r.fill_rounded_rect({box.x + 2, box.bottom() - 4.0f, box.w - 4, 3.0f}, 1.5f,
                            fade(mix(pal::accent, pal::error, wrong), wrong > 0 ? 1.0f : pulse));
    }

    const auto& lines = s.lines();
    for (std::size_t i = 0; i < lines.size(); ++i) {
        const float y = line_y(lines[i], i) + scroll_;
        if (y > area_.bottom()) break;
        draw_line(r, s, lines[i], y, f.mono, paused);
    }
    r.set_clip(std::nullopt);
}

}  // namespace ks
