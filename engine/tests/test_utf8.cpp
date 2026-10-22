#include <doctest/doctest.h>
#include <vs/engine/utf8.hpp>

TEST_CASE("utf8 decodes ASCII and Cyrillic") {
    const std::u32string cps = vs::utf8_decode("Aб\xD0\x81!");  // A, б, Ё, !
    REQUIRE(cps.size() == 4);
    CHECK(cps[0] == U'A');
    CHECK(cps[1] == U'б');
    CHECK(cps[2] == U'Ё');
    CHECK(cps[3] == U'!');
}

TEST_CASE("utf8 round trip for 1-4 byte sequences") {
    const std::u32string in = {U'z', U'é', U'я', U'₽', U'\U0001F600'};
    const std::string enc = vs::utf8_encode(in);
    CHECK(enc.size() == 1 + 2 + 2 + 3 + 4);
    CHECK(vs::utf8_decode(enc) == in);
    CHECK(vs::utf8_length(enc) == 5);
}

TEST_CASE("utf8 malformed input yields replacement characters") {
    CHECK(vs::utf8_decode("\xFF") == std::u32string{vs::kReplacementChar});
    CHECK(vs::utf8_decode("\xD0") == std::u32string{vs::kReplacementChar});           // truncated
    CHECK(vs::utf8_decode("\xC0\xAF") == std::u32string(2, vs::kReplacementChar));    // overlong
    CHECK(vs::utf8_decode("\xD0" "A") == std::u32string{vs::kReplacementChar, U'A'});
}

TEST_CASE("utf8 pop_back removes one code point") {
    std::string s = "Да";
    vs::utf8_pop_back(s);
    CHECK(s == "Д");
    vs::utf8_pop_back(s);
    CHECK(s.empty());
    vs::utf8_pop_back(s);
    CHECK(s.empty());
}

TEST_CASE("utf8 offset and upper case") {
    const std::string s = "aбc";
    CHECK(vs::utf8_offset(s, 0) == 0);
    CHECK(vs::utf8_offset(s, 1) == 1);
    CHECK(vs::utf8_offset(s, 2) == 3);
    CHECK(vs::utf8_offset(s, 9) == s.size());
    CHECK(vs::to_upper(U'q') == U'Q');
    CHECK(vs::to_upper(U'ё') == U'Ё');  // ё -> Ё
    CHECK(vs::to_upper(U'я') == U'Я');  // я -> Я
    CHECK(vs::to_upper(U'7') == U'7');
}
