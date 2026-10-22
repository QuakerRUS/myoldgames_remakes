#pragma once
// A typing session: the current and the next sentence, word wrap, cursor, statistics.

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <kbsim/layout.hpp>
#include <kbsim/text_source.hpp>

namespace kbsim {

enum class TypeResult : std::uint8_t {
    Correct,  // the expected character: the cursor moved (and maybe the sentence completed)
    Wrong,    // another character: counted as an error, the cursor stays
    Ignored,  // paused, or a control character (< U+0020, U+007F): nothing changes
};

// One visual line of the text field. Lines never copy text: `text` points into the session's
// sentences and stays valid until the next type()/restart()/wrap change.
struct Line {
    int sentence = 0;         // 0 = the current sentence, 1 = the next one
    std::size_t begin = 0;    // index of the first character in the sentence
    std::size_t end = 0;      // one past the last visible character
    bool break_space = false; // the line was wrapped at a space; that space (index `end`) is not
                              // drawn but must be typed — the cursor sits right after the text
    std::u32string_view text; // the visible characters [begin, end)
};

// What to press next (for the hint on the keyboard).
struct Expected {
    char32_t ch = 0;
    KeyIndex key = kNoKey;        // kNoKey only if the character is not on the layout
    KeyIndex shift_key = kNoKey;  // the Shift to hold, kNoKey if none
    Finger finger = Finger::Thumbs;
};

// The latest wrong key press (for the flash on the keyboard).
struct WrongPress {
    char32_t typed = 0;           // what was typed
    char32_t expected = 0;        // what was expected
    KeyIndex key = kNoKey;        // the key of `typed`; kNoKey if not on the layout (e.g. a Latin
                                  // letter: the system keyboard layout is probably not Russian)
    KeyIndex shift_key = kNoKey;  // the Shift of `typed`, if any
    std::int64_t at_ms = 0;       // the now_ms passed to type()
    std::uint64_t serial = 0;     // 1, 2, 3, ... — increases with every wrong press
};

class Session {
public:
    // A measure function returns the width of a piece of text in the renderer's units.
    using Measure = std::function<float(std::u32string_view)>;

    // Speed is reported as 0 until this much active time has passed (as in the original,
    // which showed the speed after 2 seconds).
    static constexpr std::int64_t kMinSpeedWindowMs = 2000;
    // Line length of the original program, in characters.
    static constexpr int kOriginalColumns = 41;

    // Draws the current and the next sentence from `source`. `layout` must outlive the session.
    Session(TextSource source, const Layout& layout, int columns = kOriginalColumns);
    // Movable (lines() stay valid), not copyable.
    Session(Session&&) noexcept = default;
    Session& operator=(Session&&) noexcept = default;
    Session(const Session&) = delete;
    Session& operator=(const Session&) = delete;

    // Word wrap by a character count: a line holds at most `columns` characters including the
    // break space at its end.
    void wrap_columns(int columns);
    // Word wrap by measured width: measure(line text + break space) <= max_width.
    void wrap_measure(Measure measure, float max_width);

    // The lines of the current sentence followed by the lines of the next one.
    const std::vector<Line>& lines() const { return lines_; }
    const std::u32string& current() const { return *current_; }
    const std::u32string& next() const { return *next_; }

    // Cursor: index into current(); always < current().size().
    std::size_t cursor() const { return cursor_; }
    // Index into lines() and the column within that line (column == text.size() when the
    // cursor is on the break space at the end of the line).
    int cursor_line() const;
    std::size_t cursor_column() const;

    Expected expected() const;

    // Feeds one typed character. `now_ms` is a monotonic clock in milliseconds.
    // On the last character of the sentence (. ! ?) the next sentence moves up and a new
    // one is drawn from the source; lines() are rebuilt.
    TypeResult type(char32_t ch, std::int64_t now_ms);

    // Pause stops the clock and makes type() ignore input. Both are idempotent.
    void pause(std::int64_t now_ms);
    void resume(std::int64_t now_ms);
    void toggle_pause(std::int64_t now_ms) { paused_ ? resume(now_ms) : pause(now_ms); }
    bool paused() const { return paused_; }

    // New sentences, cursor at 0, statistics and clock reset, not paused.
    void restart(std::int64_t now_ms);

    // Statistics.
    bool started() const { return started_; }              // a key was pressed since (re)start
    std::uint64_t typed() const { return typed_; }         // correct key presses
    std::uint64_t errors() const { return errors_; }       // wrong key presses
    std::uint64_t sentences_completed() const { return sentences_completed_; }
    double accuracy() const;                               // percent, 100 before any key press
    std::int64_t elapsed_ms(std::int64_t now_ms) const;    // active time since the first key press
    double speed_cpm(std::int64_t now_ms) const;           // correct characters per minute

    const std::optional<WrongPress>& last_wrong() const { return last_wrong_; }

    const Layout& layout() const { return *layout_; }

private:
    void rewrap();
    void wrap_sentence(int index, const std::u32string& s);
    bool fits(const std::u32string& s, std::size_t begin, std::size_t end, bool with_space) const;

    TextSource source_;
    const Layout* layout_;
    int columns_;
    Measure measure_;
    float max_width_ = 0;

    // Heap-held so that the views in lines_ survive moving the Session.
    std::unique_ptr<std::u32string> current_;
    std::unique_ptr<std::u32string> next_;
    std::vector<Line> lines_;
    std::size_t cursor_ = 0;

    bool paused_ = false;
    bool started_ = false;
    std::int64_t start_ms_ = 0;
    std::int64_t paused_total_ms_ = 0;
    std::int64_t pause_start_ms_ = 0;

    std::uint64_t typed_ = 0;
    std::uint64_t errors_ = 0;
    std::uint64_t sentences_completed_ = 0;
    std::uint64_t wrong_serial_ = 0;
    std::optional<WrongPress> last_wrong_;
};

}  // namespace kbsim
