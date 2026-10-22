#pragma once
#include <cstddef>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace vs {

// Byte range [begin, end) of one wrapped line inside the original text.
struct LineRange {
    std::size_t begin = 0;
    std::size_t end = 0;
    bool operator==(const LineRange&) const = default;
};

// Width of a UTF-8 string in pixels (e.g. Font::measure).
using MeasureFn = std::function<float(std::string_view)>;

// Greedy word wrap. Lines break at '\n' (hard break) and at spaces so that every line is at
// most `max_width` wide. The spaces at a soft break are not part of either line (the next
// line starts after them); a word wider than max_width is split between code points.
// max_width <= 0 disables wrapping (only '\n' breaks). An empty text gives one empty line.
std::vector<LineRange> wrap_text_ranges(std::string_view text, float max_width, const MeasureFn& measure);

// Same as wrap_text_ranges but returns copies of the lines.
std::vector<std::string> wrap_text(std::string_view text, float max_width, const MeasureFn& measure);

}  // namespace vs
