#include <doctest/doctest.h>

#include <kbsim/session.hpp>
#include <kbsim/utf8.hpp>

#include <algorithm>
#include <vector>

using namespace kbsim;

namespace {

const Layout& ru() {
    static const Layout layout = Layout::load(KBSIM_DATA_DIR "/layout/ru.json");
    return layout;
}

// A session whose current sentence is `first` (a one-text source repeats it forever).
Session single(const std::u32string& text, int columns = Session::kOriginalColumns) {
    return Session(TextSource({text}, 1), ru(), columns);
}

std::string str(std::u32string_view s) { return u32_to_utf8(s); }

// Types `text` correctly, one character per `step_ms`, starting at `t`; returns the next time.
std::int64_t type_all(Session& s, std::u32string_view text, std::int64_t t, std::int64_t step_ms = 100) {
    for (char32_t ch : text) {
        REQUIRE(s.type(ch, t) == TypeResult::Correct);
        t += step_ms;
    }
    return t;
}

// Checks that the lines are an exact, in-limit cut of the two sentences.
void check_wrap(const Session& s, int columns) {
    for (int sentence = 0; sentence < 2; ++sentence) {
        const std::u32string& text = sentence == 0 ? s.current() : s.next();
        std::u32string joined;
        for (const Line& l : s.lines()) {
            if (l.sentence != sentence) continue;
            CHECK(l.text == std::u32string_view(text).substr(l.begin, l.end - l.begin));
            CHECK(l.begin == joined.size());
            joined += l.text;
            if (l.break_space) {
                CHECK(text[l.end] == U' ');
                joined += U' ';
            }
            CHECK(l.text.size() <= static_cast<std::size_t>(columns));
            if (l.text.size() + (l.break_space ? 1 : 0) > static_cast<std::size_t>(columns)) {
                // Only a single word exactly as long as the line may leave its break space outside.
                CHECK(l.text.find(U' ') == std::u32string_view::npos);
            }
        }
        CHECK(joined == text);
    }
}

}  // namespace

TEST_CASE("session: word wrap by columns") {
    Session s = single(U"Мама мыла раму.", 10);
    REQUIRE(s.lines().size() == 4);  // two lines of the current + two of the next sentence
    CHECK(str(s.lines()[0].text) == "Мама мыла");
    CHECK(s.lines()[0].break_space);
    CHECK(str(s.lines()[1].text) == "раму.");
    CHECK_FALSE(s.lines()[1].break_space);
    CHECK(s.lines()[1].sentence == 0);
    CHECK(s.lines()[2].sentence == 1);
    check_wrap(s, 10);

    s.wrap_columns(41);
    REQUIRE(s.lines().size() == 2);
    CHECK(str(s.lines()[0].text) == "Мама мыла раму.");

    s.wrap_columns(5);  // "Мама " fits exactly with its break space
    REQUIRE(s.lines().size() == 6);
    CHECK(str(s.lines()[0].text) == "Мама");
    CHECK(str(s.lines()[1].text) == "мыла");
    CHECK(str(s.lines()[2].text) == "раму.");
    check_wrap(s, 5);
}

TEST_CASE("session: a word longer than a line is cut") {
    Session s = single(U"Абвгдежз и.", 4);
    CHECK(str(s.lines()[0].text) == "Абвг");
    CHECK_FALSE(s.lines()[0].break_space);
    CHECK(str(s.lines()[1].text) == "дежз");
    CHECK(s.lines()[1].break_space);
    CHECK(str(s.lines()[2].text) == "и.");
    check_wrap(s, 4);
    std::int64_t t = type_all(s, U"Абвг", 0);
    CHECK(s.cursor_line() == 1);
    CHECK(s.cursor_column() == 0);
    (void)t;
}

TEST_CASE("session: word wrap by a measure function") {
    Session s = single(U"Мама мыла раму.");
    int calls = 0;
    s.wrap_measure([&calls](std::u32string_view t) { ++calls; return 7.5f * static_cast<float>(t.size()); }, 75.0f);
    CHECK(calls > 0);
    REQUIRE(s.lines().size() == 4);
    CHECK(str(s.lines()[0].text) == "Мама мыла");
    CHECK(str(s.lines()[1].text) == "раму.");
    // The measured text of a wrapped line includes its break space.
    std::vector<std::u32string> measured;
    s.wrap_measure([&](std::u32string_view t) { measured.emplace_back(t); return t.size() <= 10 ? 1.0f : 2.0f; }, 1.0f);
    CHECK(str(s.lines()[0].text) == "Мама мыла");
    CHECK(s.lines()[0].break_space);
    CHECK(std::find(measured.begin(), measured.end(), U"Мама мыла ") != measured.end());
}

TEST_CASE("session: the space at the end of a visual line must be typed") {
    Session s = single(U"Мама мыла раму.", 10);
    std::int64_t t = type_all(s, U"Мама мыла", 0);
    CHECK(s.cursor() == 9);
    CHECK(s.cursor_line() == 0);
    CHECK(s.cursor_column() == 9);  // right after the visible text of line 0
    const Expected e = s.expected();
    CHECK(e.ch == U' ');
    CHECK(s.layout().key(e.key).code == "Space");
    CHECK(e.shift_key == kNoKey);
    CHECK(e.finger == Finger::Thumbs);
    CHECK(s.type(U'р', t) == TypeResult::Wrong);  // the next line's letter is not accepted
    CHECK(s.cursor() == 9);
    CHECK(s.type(U' ', t) == TypeResult::Correct);
    CHECK(s.cursor_line() == 1);
    CHECK(s.cursor_column() == 0);
    CHECK(s.expected().ch == U'р');
}

TEST_CASE("session: sentence advance") {
    Session s(TextSource({U"Да.", U"Нет!", U"Как?"}, 3), ru());
    for (int round = 0; round < 20; ++round) {
        const std::u32string next = s.next();
        CHECK(s.current() != next);
        std::int64_t t = type_all(s, s.current().substr(0, s.current().size() - 1), round * 10000);
        CHECK(s.sentences_completed() == static_cast<std::uint64_t>(round));
        const char32_t last = s.current().back();
        CHECK(s.type(last, t) == TypeResult::Correct);
        CHECK(s.sentences_completed() == static_cast<std::uint64_t>(round + 1));
        CHECK(s.current() == next);  // the next sentence moved up
        CHECK(s.next() != next);     // and a new one was added
        CHECK(s.cursor() == 0);
        CHECK(s.cursor_line() == 0);
        CHECK(s.lines().front().sentence == 0);
        CHECK(s.lines().back().sentence == 1);
    }
}

TEST_CASE("session: capital Е, Ё, digits, 0 and shifted digit-row symbols") {
    const std::u32string text = U"Ее Ёё 0 10 1234567890 !\"№;%:?*()_-+= \\/, Ок.";
    Session s = single(text, 80);
    std::int64_t t = 0;
    for (char32_t ch : text) {
        CAPTURE(str(std::u32string(1, ch)));
        const Expected e = s.expected();
        CHECK(e.ch == ch);
        REQUIRE(e.key != kNoKey);
        const Key& k = s.layout().key(e.key);
        CHECK((k.normal == ch || k.shifted == ch));
        CHECK((e.shift_key != kNoKey) == (k.shifted == ch));
        CHECK(s.type(ch, t += 50) == TypeResult::Correct);
    }
    CHECK(s.errors() == 0);
    CHECK(s.typed() == text.size());
    CHECK(s.sentences_completed() == 1);

    Session e = single(U"Ель.");
    const Expected x = e.expected();
    CHECK(e.layout().key(x.key).code == "KeyT");
    CHECK(e.layout().key(x.shift_key).code == "ShiftRight");
    CHECK(x.finger == Finger::LeftIndex);
    CHECK(e.type(U'е', 0) == TypeResult::Wrong);  // lower case is not the capital
    CHECK(e.type(U'Ё', 0) == TypeResult::Wrong);  // Ё is not Е
    CHECK(e.type(U'Е', 0) == TypeResult::Correct);
}

TEST_CASE("session: error counting, accuracy and the last wrong key") {
    Session s = single(U"Сон.");
    CHECK(s.accuracy() == doctest::Approx(100.0));
    CHECK_FALSE(s.last_wrong().has_value());
    CHECK(s.type(U'С', 100) == TypeResult::Correct);
    CHECK(s.type(U'а', 200) == TypeResult::Wrong);
    REQUIRE(s.last_wrong().has_value());
    CHECK(s.last_wrong()->typed == U'а');
    CHECK(s.last_wrong()->expected == U'о');
    CHECK(s.layout().key(s.last_wrong()->key).code == "KeyF");
    CHECK(s.last_wrong()->shift_key == kNoKey);
    CHECK(s.last_wrong()->at_ms == 200);
    CHECK(s.last_wrong()->serial == 1);
    CHECK(s.type(U'j', 300) == TypeResult::Wrong);  // Latin: not on the layout
    CHECK(s.last_wrong()->key == kNoKey);
    CHECK(s.last_wrong()->serial == 2);
    CHECK(s.type(U'О', 400) == TypeResult::Wrong);  // capital where a small letter is expected
    CHECK(s.layout().key(s.last_wrong()->shift_key).code == "ShiftLeft");
    CHECK(s.cursor() == 1);
    CHECK(s.type(U'\r', 500) == TypeResult::Ignored);
    CHECK(s.type(U'\n', 500) == TypeResult::Ignored);
    CHECK(s.type(U'\b', 500) == TypeResult::Ignored);
    CHECK(s.errors() == 3);
    CHECK(s.typed() == 1);
    CHECK(s.accuracy() == doctest::Approx(25.0));
    type_all(s, U"он.", 600);
    CHECK(s.typed() == 4);
    CHECK(s.errors() == 3);
    CHECK(s.accuracy() == doctest::Approx(400.0 / 7.0));
}

TEST_CASE("session: speed in characters per minute, excluding pauses") {
    Session s = single(U"Абвгдежзий клмн.");
    CHECK(s.elapsed_ms(5000) == 0);  // the clock starts with the first key press
    CHECK(s.speed_cpm(5000) == 0.0);
    std::int64_t t = type_all(s, U"Абвгдежзий", 1000, 100);  // 10 characters at 1000..1900
    CHECK(s.started());
    CHECK(t == 2000);
    CHECK(s.elapsed_ms(2000) == 1000);
    CHECK(s.speed_cpm(2000) == 0.0);  // less than kMinSpeedWindowMs
    CHECK(s.speed_cpm(4000) == doctest::Approx(10 * 60000.0 / 3000));  // 200 cpm

    s.pause(4000);
    CHECK(s.paused());
    CHECK(s.type(U' ', 5000) == TypeResult::Ignored);
    CHECK(s.typed() == 10);
    CHECK(s.elapsed_ms(9000) == 3000);  // frozen while paused
    s.pause(9000);                      // idempotent
    s.resume(10000);
    s.resume(10500);                    // idempotent
    CHECK_FALSE(s.paused());
    CHECK(s.elapsed_ms(13000) == 6000);  // 12000 total - 6000 paused
    CHECK(s.speed_cpm(13000) == doctest::Approx(100.0));
    s.toggle_pause(13000);
    CHECK(s.paused());
    s.toggle_pause(14000);
    CHECK(s.elapsed_ms(15000) == 7000);

    // Wrong presses start the clock too but do not add to the speed.
    Session w = single(U"Да.");
    CHECK(w.type(U'Н', 1000) == TypeResult::Wrong);
    CHECK(w.started());
    CHECK(w.type(U'Д', 2000) == TypeResult::Correct);
    CHECK(w.speed_cpm(4000) == doctest::Approx(20.0));  // 1 char in 3 s

    // A pause before the first key press does not count.
    Session p = single(U"Да.");
    p.pause(0);
    p.resume(50000);
    p.type(U'Д', 60000);
    CHECK(p.elapsed_ms(63000) == 3000);
}

TEST_CASE("session: restart") {
    Session s(TextSource({U"Да.", U"Нет!", U"Как?"}, 9), ru());
    s.type(s.current()[0], 100);
    s.type(U'x', 200);
    s.pause(300);
    s.restart(400);
    CHECK_FALSE(s.paused());
    CHECK_FALSE(s.started());
    CHECK(s.typed() == 0);
    CHECK(s.errors() == 0);
    CHECK(s.sentences_completed() == 0);
    CHECK(s.cursor() == 0);
    CHECK(s.elapsed_ms(10000) == 0);
    CHECK_FALSE(s.last_wrong().has_value());
    CHECK(s.current() != s.next());
    CHECK(s.type(s.current()[0], 1000) == TypeResult::Correct);
    CHECK(s.elapsed_ms(3000) == 2000);
}

TEST_CASE("session: lines stay valid after moving the session") {
    Session a = single(U"Мама мыла раму.", 10);
    Session b = std::move(a);
    CHECK(str(b.lines()[0].text) == "Мама мыла");
    check_wrap(b, 10);
}

TEST_CASE("session: the real corpus wraps and types through") {
    Session s(TextSource::load(KBSIM_DATA_DIR "/texts/ru.txt", 2026), ru());
    std::int64_t t = 0;
    for (int n = 0; n < 1500; ++n) {
        check_wrap(s, Session::kOriginalColumns);
        const std::u32string text = s.current();
        for (std::size_t i = 0; i < text.size(); ++i) {
            const Expected e = s.expected();
            REQUIRE(e.key != kNoKey);
            REQUIRE(s.type(text[i], t += 10) == TypeResult::Correct);
        }
    }
    CHECK(s.sentences_completed() == 1500);
    CHECK(s.errors() == 0);
}
