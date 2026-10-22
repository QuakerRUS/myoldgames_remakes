#include <doctest/doctest.h>

#include <kbsim/text_source.hpp>
#include <kbsim/utf8.hpp>

#include <set>

using namespace kbsim;

TEST_CASE("text source: deterministic, no immediate repeats") {
    std::vector<std::u32string> s = {U"А.", U"Б.", U"В.", U"А.", U""};
    TextSource a(s, 42), b(s, 42);
    CHECK(a.size() == 4);  // the empty string is dropped
    std::u32string prev;
    std::set<std::u32string> seen;
    for (int i = 0; i < 1000; ++i) {
        const std::u32string& x = a.next();
        CHECK(x == b.next());
        CHECK(x != prev);  // also across the duplicate text "А."
        prev = x;
        seen.insert(x);
    }
    CHECK(seen.size() == 3);
}

TEST_CASE("text source: two sentences alternate, one sentence repeats") {
    TextSource two({U"Да.", U"Нет."}, 7);
    std::u32string prev = two.next();
    for (int i = 0; i < 100; ++i) {
        const std::u32string x = two.next();
        CHECK(x != prev);
        prev = x;
    }
    TextSource one({U"Да."}, 7);
    CHECK(one.next() == U"Да.");
    CHECK(one.next() == U"Да.");
    CHECK_THROWS_AS(TextSource({U""}, 1), std::invalid_argument);
}

TEST_CASE("text source: the real corpus") {
    TextSource t = TextSource::load(KBSIM_DATA_DIR "/texts/ru.txt", 1);
    CHECK(t.size() == 5630);
    for (const auto& s : t.sentences()) {
        REQUIRE(!s.empty());
        const char32_t last = s.back();
        CHECK((last == U'.' || last == U'!' || last == U'?'));
        CHECK(s.find_first_of(U"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ\r\t") == std::u32string::npos);
    }
    CHECK_THROWS_AS(TextSource::load(KBSIM_DATA_DIR "/texts/missing.txt", 1), LoadError);
}
