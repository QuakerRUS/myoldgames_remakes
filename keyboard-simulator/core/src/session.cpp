#include <kbsim/session.hpp>

#include <stdexcept>

namespace kbsim {

Session::Session(TextSource source, const Layout& layout, int columns)
    : source_(std::move(source)), layout_(&layout), columns_(columns) {
    if (columns_ < 1) throw std::invalid_argument("Session: columns must be positive");
    current_ = std::make_unique<std::u32string>(source_.next());
    next_ = std::make_unique<std::u32string>(source_.next());
    rewrap();
}

void Session::wrap_columns(int columns) {
    if (columns < 1) throw std::invalid_argument("Session: columns must be positive");
    columns_ = columns;
    measure_ = nullptr;
    rewrap();
}

void Session::wrap_measure(Measure measure, float max_width) {
    if (!measure) throw std::invalid_argument("Session: empty measure function");
    measure_ = std::move(measure);
    max_width_ = max_width;
    rewrap();
}

bool Session::fits(const std::u32string& s, std::size_t begin, std::size_t end, bool with_space) const {
    const std::size_t len = end - begin + (with_space ? 1 : 0);
    if (measure_) return measure_(std::u32string_view(s).substr(begin, len)) <= max_width_;
    return len <= static_cast<std::size_t>(columns_);
}

void Session::wrap_sentence(int index, const std::u32string& s) {
    const std::size_t n = s.size();
    std::size_t pos = 0;
    while (pos < n) {
        // Greedy: take words while the line (with its break space) fits.
        std::size_t best_break = std::u32string::npos;  // index of the space to break at
        bool rest_fits = false;
        std::size_t i = pos;
        for (;;) {
            std::size_t j = s.find(U' ', i);
            if (j == std::u32string::npos) {
                rest_fits = fits(s, pos, n, false);
                break;
            }
            if (j > pos && fits(s, pos, j, true)) {
                best_break = j;
                i = j + 1;
                continue;
            }
            if (j == pos) {  // a space at the start of a line (double spaces): keep it on this line
                i = j + 1;
                continue;
            }
            break;
        }
        Line line;
        line.sentence = index;
        line.begin = pos;
        if (rest_fits) {
            line.end = n;
            pos = n;
        } else if (best_break != std::u32string::npos) {
            line.end = best_break;
            line.break_space = true;
            pos = best_break + 1;
        } else {
            // The first word does not fit even alone. If it fits without its trailing space,
            // keep it whole (the break space overhangs); otherwise cut it.
            std::size_t word_end = s.find(U' ', pos);
            if (word_end == std::u32string::npos) word_end = n;
            if (fits(s, pos, word_end, false)) {
                line.end = word_end;
                line.break_space = word_end < n;
                pos = word_end + (word_end < n ? 1 : 0);
            } else {
                std::size_t k = pos + 1;  // at least one character per line
                while (k < word_end && fits(s, pos, k + 1, false)) ++k;
                line.end = k;
                pos = k;
            }
        }
        line.text = std::u32string_view(s).substr(line.begin, line.end - line.begin);
        lines_.push_back(line);
    }
}

void Session::rewrap() {
    lines_.clear();
    wrap_sentence(0, *current_);
    wrap_sentence(1, *next_);
}

int Session::cursor_line() const {
    int last = 0;
    for (std::size_t i = 0; i < lines_.size(); ++i) {
        const Line& l = lines_[i];
        if (l.sentence != 0) break;
        last = static_cast<int>(i);
        if (cursor_ >= l.begin && cursor_ < l.end + (l.break_space ? 1 : 0)) return last;
    }
    return last;
}

std::size_t Session::cursor_column() const {
    return cursor_ - lines_[static_cast<std::size_t>(cursor_line())].begin;
}

Expected Session::expected() const {
    Expected e;
    e.ch = (*current_)[cursor_];
    if (const auto s = layout_->stroke(e.ch)) {
        e.key = s->key;
        e.shift_key = s->shift_key;
        e.finger = s->finger;
    }
    return e;
}

TypeResult Session::type(char32_t ch, std::int64_t now_ms) {
    if (paused_ || ch < 0x20 || ch == 0x7F) return TypeResult::Ignored;
    if (!started_) {
        started_ = true;
        start_ms_ = now_ms;
        paused_total_ms_ = 0;
    }
    const char32_t want = (*current_)[cursor_];
    if (ch != want) {
        ++errors_;
        WrongPress w;
        w.typed = ch;
        w.expected = want;
        if (const auto s = layout_->stroke(ch)) {
            w.key = s->key;
            w.shift_key = s->shift_key;
        }
        w.at_ms = now_ms;
        w.serial = ++wrong_serial_;
        last_wrong_ = w;
        return TypeResult::Wrong;
    }
    ++typed_;
    ++cursor_;
    if (cursor_ >= current_->size()) {
        // The sentence ends with . ! ? — the next one moves up, a new one is appended.
        ++sentences_completed_;
        current_ = std::move(next_);
        next_ = std::make_unique<std::u32string>(source_.next());
        cursor_ = 0;
        rewrap();
    }
    return TypeResult::Correct;
}

void Session::pause(std::int64_t now_ms) {
    if (paused_) return;
    paused_ = true;
    pause_start_ms_ = now_ms;
}

void Session::resume(std::int64_t now_ms) {
    if (!paused_) return;
    paused_ = false;
    if (started_) paused_total_ms_ += now_ms - pause_start_ms_;
}

void Session::restart(std::int64_t /*now_ms*/) {
    current_ = std::make_unique<std::u32string>(source_.next());
    next_ = std::make_unique<std::u32string>(source_.next());
    cursor_ = 0;
    paused_ = false;
    started_ = false;
    start_ms_ = paused_total_ms_ = pause_start_ms_ = 0;
    typed_ = errors_ = sentences_completed_ = 0;
    last_wrong_.reset();
    rewrap();
}

double Session::accuracy() const {
    const std::uint64_t total = typed_ + errors_;
    if (total == 0) return 100.0;
    return 100.0 * static_cast<double>(typed_) / static_cast<double>(total);
}

std::int64_t Session::elapsed_ms(std::int64_t now_ms) const {
    if (!started_) return 0;
    std::int64_t active = now_ms - start_ms_ - paused_total_ms_;
    if (paused_) active -= now_ms - pause_start_ms_;
    return active > 0 ? active : 0;
}

double Session::speed_cpm(std::int64_t now_ms) const {
    const std::int64_t ms = elapsed_ms(now_ms);
    if (ms < kMinSpeedWindowMs) return 0.0;
    return static_cast<double>(typed_) * 60000.0 / static_cast<double>(ms);
}

}  // namespace kbsim
