#include <doctest/doctest.h>

#include <kbsim/utf8.hpp>

#include "json.hpp"

using namespace kbsim;

TEST_CASE("utf8 round trip") {
    const std::string s = "Ёж, \xE2\x84\x96 5! \xF0\x9F\x98\x80";  // Ёж, № 5! U+1F600
    const std::u32string u = utf8_to_u32(s);
    CHECK(u == U"Ёж, № 5! \U0001F600");
    CHECK(u32_to_utf8(u) == s);
    CHECK(utf8_to_u32("\xEF\xBB\xBFok") == U"ok");
}

TEST_CASE("utf8 rejects malformed input") {
    CHECK_THROWS_AS(utf8_to_u32("\xD0"), std::invalid_argument);          // truncated
    CHECK_THROWS_AS(utf8_to_u32("\xC0\x80"), std::invalid_argument);      // overlong
    CHECK_THROWS_AS(utf8_to_u32("\xED\xA0\x80"), std::invalid_argument);  // surrogate
    CHECK_THROWS_AS(utf8_to_u32("\x80"), std::invalid_argument);          // stray continuation
    CHECK_THROWS_AS(utf8_to_u32("\xD0\x41"), std::invalid_argument);      // bad continuation
}

TEST_CASE("json parser") {
    const json::Value v = json::parse(R"({"a": [1, 2.5, -3e2], "b": "xё\"", "c": true, "d": null, "e": {}})");
    const json::Array& a = v.find("a")->as_array();
    REQUIRE(a.size() == 3);
    CHECK(a[1].as_number() == 2.5);
    CHECK(a[2].as_number() == -300);
    CHECK(v.find("b")->as_string() == "x\xD1\x91\"");
    CHECK(v.find("c")->as_bool());
    CHECK(v.find("d")->is_null());
    CHECK(v.find("e")->as_object().empty());
    CHECK(v.find("zz") == nullptr);
    CHECK_THROWS(json::parse("{\"a\": }"));
    CHECK_THROWS(json::parse("[1, 2] x"));
    CHECK_THROWS(v.find("a")->as_string());
}
