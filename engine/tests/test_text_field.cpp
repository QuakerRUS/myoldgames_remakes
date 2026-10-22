#include <doctest/doctest.h>
#include <vs/engine/ui_text_field.hpp>

TEST_CASE("text field filters, upper-cases and limits length") {
    vs::Theme theme;
    vs::TextField f(&theme);
    f.max_length = 8;
    f.uppercase = true;
    f.filter = vs::TextField::latin_upper_or_digit;
    CHECK(f.insert("lisiy-1"));
    CHECK(f.text == "LISIY1");
    f.insert("234567");
    CHECK(f.text == "LISIY123");
    CHECK_FALSE(f.insert("x"));
}

TEST_CASE("text field accepts Cyrillic without a filter") {
    vs::Theme theme;
    vs::TextField f(&theme);
    f.max_length = 3;
    f.insert("Привет");
    CHECK(f.text == "При");
    f.text.clear();
    f.insert("a\tb");  // control characters are dropped
    CHECK(f.text == "ab");
}
