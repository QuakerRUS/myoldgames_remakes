#include <vs/engine/text_layout.hpp>

#include <vs/engine/utf8.hpp>

namespace vs {

namespace {

// Wraps one paragraph [begin, end) that contains no '\n'.
void wrap_paragraph(std::string_view text, std::size_t begin, std::size_t end, float max_width, const MeasureFn& measure,
                    std::vector<LineRange>& out) {
    auto width = [&](std::size_t a, std::size_t b) { return measure(text.substr(a, b - a)); };
    std::size_t line_begin = begin;
    std::size_t line_end = begin;  // end of the last word placed on the line
    bool has_word = false;
    std::size_t pos = begin;
    while (pos < end) {
        std::size_t word_begin = pos;
        while (word_begin < end && text[word_begin] == ' ') ++word_begin;
        if (word_begin == end) break;  // only trailing spaces remain
        std::size_t word_end = word_begin;
        while (word_end < end && text[word_end] != ' ') ++word_end;

        if (max_width <= 0 || width(line_begin, word_end) <= max_width) {
            line_end = word_end;
            has_word = true;
            pos = word_end;
            continue;
        }
        if (has_word) {
            // Soft break before this word; the spaces between belong to neither line.
            out.push_back({line_begin, line_end});
            line_begin = word_begin;
            line_end = word_begin;
            has_word = false;
            pos = word_begin;
            continue;
        }
        // The word alone does not fit: split it between code points.
        std::size_t cut = word_begin;
        std::size_t probe = word_begin;
        while (probe < word_end) {
            std::size_t next = probe;
            utf8_next(text, next);
            if (width(line_begin, next) > max_width) break;
            probe = next;
            cut = next;
        }
        if (cut == word_begin) utf8_next(text, cut);  // at least one code point per line
        out.push_back({line_begin, cut});
        line_begin = cut;
        line_end = cut;
        pos = cut;
    }
    // The last line keeps its trailing spaces.
    out.push_back({line_begin, end});
}

}  // namespace

std::vector<LineRange> wrap_text_ranges(std::string_view text, float max_width, const MeasureFn& measure) {
    std::vector<LineRange> out;
    std::size_t begin = 0;
    while (true) {
        std::size_t nl = text.find('\n', begin);
        std::size_t end = nl == std::string_view::npos ? text.size() : nl;
        wrap_paragraph(text, begin, end, max_width, measure, out);
        if (nl == std::string_view::npos) break;
        begin = nl + 1;
    }
    return out;
}

std::vector<std::string> wrap_text(std::string_view text, float max_width, const MeasureFn& measure) {
    std::vector<std::string> lines;
    for (const LineRange& r : wrap_text_ranges(text, max_width, measure))
        lines.emplace_back(text.substr(r.begin, r.end - r.begin));
    return lines;
}

}  // namespace vs
