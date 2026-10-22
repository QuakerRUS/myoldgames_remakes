#pragma once
#include <cstdint>
#include <vector>

#include <kbsim/layout.hpp>
#include <kbsim/session.hpp>
#include <vs/engine/renderer.hpp>

#include "fonts.hpp"

namespace ks {

// The on-screen ЙЦУКЕН keyboard with finger colours, the hint for the key to press
// (and its Shift), press / wrong-key animations and the finger legend below it.
class KeyboardView {
public:
    // `keys_area`: the keyboard is scaled to its width and placed at its top-left corner.
    KeyboardView(vs::RectF keys_area, vs::RectF legend_area, const kbsim::Layout& layout);

    void reset();
    void on_correct(kbsim::KeyIndex key, kbsim::KeyIndex shift_key);  // press animation
    void update(double dt);
    void render(vs::Renderer& r, const Fonts& f, const kbsim::Session& s, std::int64_t now_ms, double time_s) const;

private:
    struct Hint {
        kbsim::KeyIndex key = kbsim::kNoKey;
        kbsim::KeyIndex shift = kbsim::kNoKey;
        kbsim::KeyIndex wrong = kbsim::kNoKey;
        float wrong_t = 0;  // 1 = just pressed, fades to 0
        float pulse = 0;    // 0..1 breathing of the highlight
    };
    void draw_key(vs::Renderer& r, const Fonts& f, kbsim::KeyIndex k, const Hint& h) const;
    void draw_label(vs::Renderer& r, const Fonts& f, const kbsim::Key& key, vs::RectF face, vs::Color ink) const;
    void draw_legend(vs::Renderer& r, const Fonts& f, const kbsim::Session& s, const Hint& h) const;

    const kbsim::Layout* layout_;
    vs::RectF area_;
    vs::RectF legend_;
    float unit_ = 60.0f;
    std::vector<vs::RectF> rects_;  // per key, gaps applied
    std::vector<float> press_;      // per key: 1 = just pressed, decays to 0
};

}  // namespace ks
