#pragma once
#include <cstdint>
#include <string>

#include <kbsim/session.hpp>
#include <vs/engine/renderer.hpp>

#include "fonts.hpp"

namespace ks {

// «Скорость: N зн/мин», «Ошибки: N», «Точность: N%», «Время: mm:ss» in four equal columns.
void draw_stats_bar(vs::Renderer& r, const Fonts& f, vs::RectF rect, const kbsim::Session& s, std::int64_t now_ms);

std::string format_time(std::int64_t ms);  // mm:ss

}  // namespace ks
