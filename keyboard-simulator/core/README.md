# kbsim_core

SDL-free logic of Keyboard Simulator: the sentences, the ЙЦУКЕН layout and the typing
session. C++20, namespace `kbsim`, headers in `include/kbsim/`. Text is UTF-32
(`char32_t`, `std::u32string`) inside the library and UTF-8 in files.

```cmake
target_link_libraries(keyboard_simulator PRIVATE kbsim_core)
```

Data files (copy them next to the program or point at `keyboard-simulator/data/`):

- `data/texts/ru.txt` — 5630 sentences, see `docs/formats/kbsim-texts.md`;
- `data/layout/ru.json` — the keyboard, see `docs/formats/kbsim-layout.md`.

Errors while loading throw `kbsim::LoadError` (a `std::runtime_error`) with a readable message.

## Typical use (SDL app)

```cpp
#include <kbsim/layout.hpp>
#include <kbsim/session.hpp>
#include <kbsim/utf8.hpp>

kbsim::Layout layout = kbsim::Layout::load(data / "layout/ru.json");      // must outlive the session
kbsim::Session session(kbsim::TextSource::load(data / "texts/ru.txt", seed), layout);
session.wrap_measure([&](std::u32string_view s) { return font.width(s); }, text_box_width);

// SDL_EVENT_TEXT_INPUT: event.text.text is UTF-8, possibly several characters.
for (char32_t ch : kbsim::utf8_to_u32(event.text.text))
    session.type(ch, SDL_GetTicks());   // TypeResult::Correct / Wrong / Ignored

// Buttons
session.restart(SDL_GetTicks());        // Рестарт
session.toggle_pause(SDL_GetTicks());   // Пауза / Продолжить

// Drawing
for (std::size_t i = 0; i < session.lines().size(); ++i) { const kbsim::Line& l = session.lines()[i]; ... }
// cursor box: line session.cursor_line(), column session.cursor_column()
kbsim::Expected e = session.expected();   // highlight e.key and e.shift_key, finger e.finger
if (auto w = session.last_wrong(); w && now - w->at_ms < 300) flash(w->key);
draw_stats(session.speed_cpm(now), session.errors(), session.accuracy());
```

## `kbsim/utf8.hpp`

| Declaration | Notes |
|---|---|
| `struct LoadError : std::runtime_error` | thrown by every `load()` |
| `std::u32string utf8_to_u32(std::string_view)` | skips a BOM; throws `std::invalid_argument` on malformed UTF-8 |
| `std::string u32_to_utf8(std::u32string_view)` / `u32_to_utf8(char32_t)` | throws `std::invalid_argument` on invalid code points |
| `std::string read_file(const std::filesystem::path&)` | whole file as bytes; throws `LoadError` |

## `kbsim/text_source.hpp` — `class TextSource`

| Member | Notes |
|---|---|
| `TextSource(std::vector<std::u32string> sentences, std::uint64_t seed)` | drops empty strings; throws `std::invalid_argument` if none is left |
| `static TextSource load(const std::filesystem::path&, std::uint64_t seed)` | UTF-8, one sentence per line, LF or CRLF, empty lines skipped; throws `LoadError` |
| `std::size_t size() const`, `const std::u32string& sentence(std::size_t) const`, `const std::vector<std::u32string>& sentences() const` | |
| `const std::u32string& next()` | random sentence (`std::mt19937_64`, the same seed gives the same order everywhere); never the same text as the one returned just before (unless the source has a single distinct text) |
| `void reseed(std::uint64_t seed)` | |

Use e.g. the current time as the seed in the app.

## `kbsim/layout.hpp`

```cpp
enum class Finger : std::uint8_t { LeftPinky, LeftRing, LeftMiddle, LeftIndex, Thumbs,
                                   RightIndex, RightMiddle, RightRing, RightPinky };
inline constexpr int kFingerCount = 9;   // legend order "5 4 3 2 1 / 1 2 3 4 5" (Thumbs = both 1s)
enum class Hand : std::uint8_t { Left, Right, Both };
Hand hand_of(Finger);
int legend_number(Finger);               // 5 4 3 2 1 2 3 4 5
std::string_view finger_id(Finger);      // "left_pinky" ... as in the JSON
std::optional<Finger> finger_from_id(std::string_view);

using KeyIndex = int;                    // index into Layout::keys()
inline constexpr KeyIndex kNoKey = -1;

struct Key {
    std::string code;        // "KeyQ", "Digit1", "ShiftLeft", "Space", ... (W3C KeyboardEvent.code)
    char32_t normal = 0;     // without Shift, 0 if none
    char32_t shifted = 0;    // with Shift, 0 if none
    std::string label;       // UTF-8 caption; empty: draw `normal` bottom-left and `shifted`
                             // top-right (symbol keys); Space has an empty label and normal ' '
    int row = 0;             // 0..4, top to bottom
    float x = 0;             // left edge, key units from the keyboard's left edge
    float width = 1;         // key units; height is always 1 unit
    Finger finger;
    bool home = false;       // ФЫВА ОЛДЖ
    bool bump = false;       // А and О
    KeyIndex shift_with = kNoKey;   // Shift for `shifted` (opposite hand)
};

struct Stroke { char32_t ch; KeyIndex key; bool shift; KeyIndex shift_key; Finger finger; };

class Layout {
public:
    static Layout load(const std::filesystem::path&);   // throws LoadError
    static Layout parse(std::string_view json_text);    // throws LoadError
    const std::string& name() const;                    // "ЙЦУКЕН"
    const std::vector<Key>& keys() const;
    const Key& key(KeyIndex) const;
    int row_count() const;                               // 5
    const std::vector<KeyIndex>& row(int r) const;       // left to right
    float row_width(int r) const;                        // 15 for every row of ru.json
    float width() const;                                 // widest row: 15
    KeyIndex find_code(std::string_view code) const;     // kNoKey if absent
    std::optional<Stroke> stroke(char32_t ch) const;     // nullopt: not on the layout
    bool can_type(char32_t ch) const;
};
```

Drawing the keyboard: key `k` occupies `[k.x, k.x + k.width] x [k.row, k.row + 1]` in key
units; scale by `keyboard_width / layout.width()` and leave a small gap between keys. Colour
keys by `k.finger` (the original colours are listed in `docs/formats/kbsim-layout.md`). The
finger legend below the keyboard is `legend_number()` of `LeftPinky..LeftIndex`, `Thumbs`
twice, `RightIndex..RightPinky`. The hint highlights `expected().key`, plus
`expected().shift_key` for capitals and shifted symbols (left-hand keys use the right Shift
and vice versa).

## `kbsim/session.hpp` — `class Session`

```cpp
enum class TypeResult : std::uint8_t { Correct, Wrong, Ignored };

struct Line {
    int sentence;            // 0 = current sentence, 1 = next sentence
    std::size_t begin, end;  // [begin, end) indices of the visible characters in that sentence
    bool break_space;        // wrapped at a space: the space (index `end`) is not drawn but must be typed
    std::u32string_view text;// the visible characters
};
struct Expected { char32_t ch; KeyIndex key; KeyIndex shift_key; Finger finger; };
struct WrongPress { char32_t typed; char32_t expected; KeyIndex key; KeyIndex shift_key;
                    std::int64_t at_ms; std::uint64_t serial; };
```

| Member | Notes |
|---|---|
| `Session(TextSource source, const Layout& layout, int columns = 41)` | draws the current and the next sentence; wraps at `columns` until told otherwise. Movable (lines stay valid), not copyable. `layout` must outlive the session. |
| `void wrap_columns(int columns)` | a line holds at most `columns` characters including its break space |
| `void wrap_measure(std::function<float(std::u32string_view)> measure, float max_width)` | a line fits if `measure(visible text + break space) <= max_width`; call again when the window is resized |
| `const std::vector<Line>& lines() const` | lines of the current sentence, then of the next one. Views are invalidated by `type()`, `restart()` and the `wrap_*` calls; re-read after each. |
| `const std::u32string& current() const`, `next() const` | |
| `std::size_t cursor() const` | index into `current()`, always `< current().size()` |
| `int cursor_line() const`, `std::size_t cursor_column() const` | where to draw the cursor box; `cursor_column() == text.size()` means the break space at the end of the line |
| `Expected expected() const` | the character to type, its key, the Shift to hold (`kNoKey` if none) and the finger |
| `TypeResult type(char32_t ch, std::int64_t now_ms)` | `Correct`: the cursor moves; after the last character (`.` `!` `?`) the next sentence moves up, a new one is appended, `sentences_completed()` grows and `lines()` are rebuilt. `Wrong`: counted, `last_wrong()` updated, the cursor stays. `Ignored`: paused or a control character (`< U+0020`, `U+007F`). The match is exact: `е` is not `Е`, `е` is not `ё`. |
| `void pause(now_ms)`, `void resume(now_ms)`, `void toggle_pause(now_ms)`, `bool paused() const` | the clock stops while paused; idempotent |
| `void restart(now_ms)` | two new sentences, cursor 0, all statistics reset, not paused |
| `bool started() const` | a key was pressed since the start/restart |
| `std::uint64_t typed() const` | correct key presses |
| `std::uint64_t errors() const` | wrong key presses (each one counts) |
| `std::uint64_t sentences_completed() const` | |
| `double accuracy() const` | `100 * typed / (typed + errors)`; 100 before any key press |
| `std::int64_t elapsed_ms(now_ms) const` | active time since the first key press (correct or wrong), pauses excluded |
| `double speed_cpm(now_ms) const` | `typed * 60000 / elapsed_ms`; 0 while `elapsed_ms < kMinSpeedWindowMs` (2000) |
| `const std::optional<WrongPress>& last_wrong() const` | the latest wrong press; `key == kNoKey` if the typed character is not on the layout (e.g. Latin: the system layout is probably not Russian — worth a hint). Flash `key` while `now - at_ms` is small; `serial` tells a new press from the same one. |
| `const Layout& layout() const` | |
| `static constexpr std::int64_t kMinSpeedWindowMs = 2000`, `static constexpr int kOriginalColumns = 41` | |

`now_ms` is any monotonic millisecond clock (`SDL_GetTicks()`); the session never reads a
clock itself.

### Behaviour compared with the 2007 original

- Same: a wrong key does not move the cursor; the space at the end of a visual line must be
  typed; after `.` `!` `?` the sentence leaves and the next one moves up; 41 columns by
  default; speed is correct characters per minute, shown after 2 seconds.
- Fixed: the capital Е and `0` can be typed, and digits are no longer off by one (the original
  showed and expected "2" for "1"); input is the actual character, not a QWERTY code; errors and accuracy are counted; the clock has millisecond
  precision and stops while paused; a word longer than a line is cut instead of hanging.
- A sentence completes at its last character. (The original stopped at the first `.` `!` `?`;
  no sentence of `ru.txt` has one before its end, so the result is the same.)

## Tests

`kbsim_core_tests` (doctest): UTF-8 and JSON, the text source, every lookup and Shift side
of the layout, every character of `ru.txt` typeable, word wrap (columns and measure), the
line-end space, sentence advance, capital Е / Ё / digits / `0`, error counting, speed with
pause, restart, and 1500 real sentences typed through.

```
cmake --preset vs2022 -B build/kbsim-core -DVS_LOGIC_ONLY=ON
cmake --build build/kbsim-core --config Debug --target kbsim_core_tests
ctest --test-dir build/kbsim-core -C Debug -R kbsim
```
