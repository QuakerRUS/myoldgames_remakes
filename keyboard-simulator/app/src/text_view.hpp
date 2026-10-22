#pragma once
#include <cstdint>

#include <kbsim/session.hpp>
#include <vs/engine/font.hpp>
#include <vs/engine/renderer.hpp>

#include "fonts.hpp"

namespace ks {

// The sentences: the current one (typed part coloured, cursor box on the letter to type)
// and the next one dimmed below it. Animates the cursor and the scroll after a sentence.
class TextView {
public:
    explicit TextView(vs::RectF area) : area_(area) {}

    vs::RectF area() const { return area_; }
    float wrap_width() const { return area_.w - 6.0f; }
    float line_height() const { return line_h_; }

    void reset();                               // after Restart: no animation in flight
    void on_sentence_done(int lines_scrolled);  // the text glides up by these lines
    void update(double dt, const kbsim::Session& s, const vs::Font& mono);
    void render(vs::Renderer& r, const kbsim::Session& s, const Fonts& f, std::int64_t now_ms,
                double time_s) const;

private:
    float line_y(const kbsim::Line& line, std::size_t index) const;  // without scroll
    vs::RectF cursor_target(const kbsim::Session& s, const vs::Font& mono) const;
    void draw_line(vs::Renderer& r, const kbsim::Session& s, const kbsim::Line& line, float y,
                   const vs::Font& mono, bool paused) const;

    vs::RectF area_;
    float line_h_ = 36.0f;
    float sentence_gap_ = 8.0f;
    float scroll_ = 0.0f;
    vs::RectF cursor_{};
    bool cursor_valid_ = false;
};

}  // namespace ks
