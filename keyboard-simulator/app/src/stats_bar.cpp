#include "stats_bar.hpp"

#include <array>
#include <cmath>
#include <cstdio>
#include <string>

#include "palette.hpp"

namespace ks {

namespace {
std::string format_accuracy(double acc) {
    // Floor, so that 100% means no errors at all.
    const long long v = acc >= 100.0 ? 100 : static_cast<long long>(std::floor(acc));
    return std::to_string(v) + "%";
}
}  // namespace

std::string format_time(std::int64_t ms) {
    const long long total = ms / 1000;
    char buf[32];
    std::snprintf(buf, sizeof buf, "%02lld:%02lld", total / 60, total % 60);
    return buf;
}

void draw_stats_bar(vs::Renderer& r, const Fonts& f, vs::RectF rect, const kbsim::Session& s, std::int64_t now_ms) {
    struct Item {
        const char* label;
        std::string value;
        vs::Color dot;
    };
    const std::array<Item, 4> items = {{
        {"Скорость:", std::to_string(std::llround(s.speed_cpm(now_ms))) + " зн/мин", pal::accent},
        {"Ошибки:", std::to_string(s.errors()), pal::error},
        {"Точность:", format_accuracy(s.accuracy()), pal::typed},
        {"Время:", format_time(s.elapsed_ms(now_ms)), pal::finger_style(kbsim::Finger::LeftRing).accent},
    }};
    const float col_w = rect.w / static_cast<float>(items.size());
    const float cy = rect.y + rect.h * 0.5f;
    for (std::size_t i = 0; i < items.size(); ++i) {
        const Item& it = items[i];
        float x = rect.x + col_w * static_cast<float>(i);
        r.fill_circle({x + 5.0f, cy + 1.0f}, 4.5f, it.dot);
        x += 18.0f;
        vs::TextStyle label;
        label.color = pal::muted;
        const vs::Vec2 lw = f.ui.draw(r, it.label, {x, cy - f.ui.line_height() * 0.5f}, label);
        vs::TextStyle value;
        value.color = pal::ink;
        f.ui_bold.draw(r, it.value, {x + lw.x + 6.0f, cy - f.ui_bold.line_height() * 0.5f}, value);
    }
}

}  // namespace ks
