#include <doctest/doctest.h>
#include <vs/engine/text_layout.hpp>
#include <vs/engine/utf8.hpp>

namespace {
// Fake metric: every code point is 10 px wide.
float fake(std::string_view s) { return static_cast<float>(vs::utf8_length(s)) * 10.0f; }
}  // namespace

TEST_CASE("wrap keeps short text on one line") {
    CHECK(vs::wrap_text("hello world", 200, fake) == std::vector<std::string>{"hello world"});
    CHECK(vs::wrap_text("", 200, fake) == std::vector<std::string>{""});
}

TEST_CASE("wrap breaks at spaces and drops the breaking spaces") {
    // "aaa bbb ccc" with 70 px -> "aaa bbb" (70) fits, "ccc" next.
    CHECK(vs::wrap_text("aaa bbb ccc", 70, fake) == std::vector<std::string>{"aaa bbb", "ccc"});
    CHECK(vs::wrap_text("aaa  bbb", 50, fake) == std::vector<std::string>{"aaa", "bbb"});
    const auto r = vs::wrap_text_ranges("aaa bbb ccc", 70, fake);
    REQUIRE(r.size() == 2);
    CHECK(r[0] == vs::LineRange{0, 7});
    CHECK(r[1] == vs::LineRange{8, 11});
}

TEST_CASE("wrap honours hard line breaks") {
    CHECK(vs::wrap_text("ab\ncd\n", 1000, fake) == std::vector<std::string>{"ab", "cd", ""});
}

TEST_CASE("wrap splits a word wider than the line") {
    CHECK(vs::wrap_text("abcdefg", 30, fake) == std::vector<std::string>{"abc", "def", "g"});
    // Cyrillic is split between code points, not bytes.
    CHECK(vs::wrap_text("абвгд", 20, fake) == std::vector<std::string>{"аб", "вг", "д"});
}

TEST_CASE("wrap with no width only breaks at newlines") {
    CHECK(vs::wrap_text("a b c", 0, fake) == std::vector<std::string>{"a b c"});
}

TEST_CASE("wrap keeps trailing spaces of the last line") {
    CHECK(vs::wrap_text("ab cd ", 100, fake) == std::vector<std::string>{"ab cd "});
}
