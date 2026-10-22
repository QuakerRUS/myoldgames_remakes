#include <doctest/doctest.h>

#include <kbsim/layout.hpp>
#include <kbsim/text_source.hpp>
#include <kbsim/utf8.hpp>

#include <set>

using namespace kbsim;

namespace {

const Layout& ru() {
    static const Layout layout = Layout::load(KBSIM_DATA_DIR "/layout/ru.json");
    return layout;
}

std::string code_of(KeyIndex k) { return k == kNoKey ? std::string("-") : ru().key(k).code; }

}  // namespace

TEST_CASE("layout: structure of ru.json") {
    const Layout& l = ru();
    CHECK(l.name() == "ЙЦУКЕН");
    REQUIRE(l.row_count() == 5);
    CHECK(l.row(0).size() == 14);
    CHECK(l.row(1).size() == 14);
    CHECK(l.row(2).size() == 13);
    CHECK(l.row(3).size() == 12);
    CHECK(l.row(4).size() == 8);
    for (int r = 0; r < l.row_count(); ++r) CHECK(l.row_width(r) == doctest::Approx(15.0));
    CHECK(l.width() == doctest::Approx(15.0));
    const Key& f = l.key(l.find_code("KeyF"));
    CHECK(f.normal == U'а');
    CHECK(f.shifted == U'А');
    CHECK(f.label == "А");
    CHECK(f.home);
    CHECK(f.bump);
    CHECK(f.row == 2);
    CHECK(f.x == doctest::Approx(4.75f));
    CHECK(l.key(l.find_code("Backspace")).label == "Backspace");
    CHECK(l.find_code("NoSuchKey") == kNoKey);
    int chars = 0;
    for (const Key& k : l.keys()) chars += (k.normal != 0) + (k.shifted != 0);
    CHECK(chars == 95);  // the 95 glyphs of the original
}

TEST_CASE("layout: lookups") {
    struct Case {
        char32_t ch;
        const char* key;
        const char* shift;
        Finger finger;
    };
    const Case cases[] = {
        {U'а', "KeyF", "-", Finger::LeftIndex},
        {U'А', "KeyF", "ShiftRight", Finger::LeftIndex},
        {U'Е', "KeyT", "ShiftRight", Finger::LeftIndex},  // the capital Е the original could not type
        {U'е', "KeyT", "-", Finger::LeftIndex},
        {U'ё', "Backquote", "-", Finger::LeftPinky},
        {U'Ё', "Backquote", "ShiftRight", Finger::LeftPinky},
        {U'о', "KeyJ", "-", Finger::RightIndex},
        {U'О', "KeyJ", "ShiftLeft", Finger::RightIndex},
        {U'ж', "Semicolon", "-", Finger::RightPinky},
        {U'Ю', "Period", "ShiftLeft", Finger::RightRing},
        {U'0', "Digit0", "-", Finger::RightPinky},  // '0' was impossible in the original
        {U'1', "Digit1", "-", Finger::LeftPinky},
        {U'!', "Digit1", "ShiftRight", Finger::LeftPinky},
        {U'"', "Digit2", "ShiftRight", Finger::LeftRing},
        {U'№', "Digit3", "ShiftRight", Finger::LeftMiddle},
        {U';', "Digit4", "ShiftRight", Finger::LeftIndex},
        {U'%', "Digit5", "ShiftRight", Finger::LeftIndex},
        {U':', "Digit6", "ShiftRight", Finger::LeftIndex},
        {U'?', "Digit7", "ShiftLeft", Finger::RightIndex},
        {U'*', "Digit8", "ShiftLeft", Finger::RightMiddle},
        {U'(', "Digit9", "ShiftLeft", Finger::RightRing},
        {U')', "Digit0", "ShiftLeft", Finger::RightPinky},
        {U'-', "Minus", "-", Finger::RightPinky},
        {U'_', "Minus", "ShiftLeft", Finger::RightPinky},
        {U'=', "Equal", "-", Finger::RightPinky},
        {U'+', "Equal", "ShiftLeft", Finger::RightPinky},
        {U'\x5C', "Backslash", "-", Finger::RightPinky},
        {U'/', "Backslash", "ShiftLeft", Finger::RightPinky},
        {U'.', "Slash", "-", Finger::RightPinky},
        {U',', "Slash", "ShiftLeft", Finger::RightPinky},
        {U' ', "Space", "-", Finger::Thumbs},
    };
    for (const Case& c : cases) {
        CAPTURE(u32_to_utf8(c.ch));
        const auto s = ru().stroke(c.ch);
        REQUIRE(s.has_value());
        CHECK(s->ch == c.ch);
        CHECK(code_of(s->key) == c.key);
        CHECK(code_of(s->shift_key) == c.shift);
        CHECK(s->shift == (std::string(c.shift) != "-"));
        CHECK(s->finger == c.finger);
    }
    CHECK_FALSE(ru().stroke(U'a').has_value());  // Latin
    CHECK_FALSE(ru().stroke(U'@').has_value());
    CHECK_FALSE(ru().can_type(U'\n'));
}

TEST_CASE("layout: Shift is always on the opposite hand") {
    const Layout& l = ru();
    const KeyIndex left = l.find_code("ShiftLeft"), right = l.find_code("ShiftRight");
    CHECK(hand_of(l.key(left).finger) == Hand::Left);
    CHECK(hand_of(l.key(right).finger) == Hand::Right);
    int shifted = 0;
    for (const Key& k : l.keys()) {
        if (k.shifted == 0) continue;
        ++shifted;
        CAPTURE(k.code);
        const auto s = l.stroke(k.shifted);
        REQUIRE(s.has_value());
        CHECK(s->shift);
        REQUIRE(hand_of(k.finger) != Hand::Both);
        CHECK(s->shift_key == (hand_of(k.finger) == Hand::Left ? right : left));
        CHECK(hand_of(l.key(s->shift_key).finger) != hand_of(k.finger));
    }
    CHECK(shifted == 47);
}

TEST_CASE("layout: every character of the corpus is typeable") {
    const TextSource t = TextSource::load(KBSIM_DATA_DIR "/texts/ru.txt", 1);
    std::set<char32_t> alphabet;
    for (const auto& s : t.sentences()) alphabet.insert(s.begin(), s.end());
    CHECK(alphabet.size() >= 80);
    for (char32_t ch : alphabet) {
        CAPTURE(u32_to_utf8(ch));
        CHECK(ru().can_type(ch));
    }
}

TEST_CASE("layout: fingers") {
    CHECK(legend_number(Finger::LeftPinky) == 5);
    CHECK(legend_number(Finger::Thumbs) == 1);
    CHECK(legend_number(Finger::RightRing) == 4);
    CHECK(hand_of(Finger::LeftIndex) == Hand::Left);
    CHECK(hand_of(Finger::RightIndex) == Hand::Right);
    for (int i = 0; i < kFingerCount; ++i) {
        const auto f = static_cast<Finger>(i);
        CHECK(finger_from_id(finger_id(f)) == f);
    }
    CHECK_FALSE(finger_from_id("left_thumb").has_value());
}

TEST_CASE("layout: malformed files are rejected") {
    const auto doc = [](const std::string& keys) {
        return R"({"format": "kbsim-layout", "version": 1, "rows": [[)" + keys + "]]}";
    };
    const std::string shift = R"({"code": "ShiftLeft", "finger": "left_pinky"})";
    CHECK_NOTHROW(Layout::parse(doc(shift + R"(, {"code": "KeyQ", "normal": "й", "shift": "Й", "finger": "left_pinky", "shift_with": "ShiftLeft"})")));
    CHECK_THROWS_AS(Layout::parse(doc(R"({"code": "KeyQ", "normal": "й", "finger": "left_toe"})")), LoadError);
    CHECK_THROWS_AS(Layout::parse(doc(R"({"code": "KeyQ", "normal": "й", "shift": "Й", "finger": "left_pinky"})")), LoadError);
    CHECK_THROWS_AS(Layout::parse(doc(R"({"code": "KeyQ", "normal": "й", "finger": "left_pinky"}, {"code": "KeyW", "normal": "й", "finger": "left_ring"})")), LoadError);
    CHECK_THROWS_AS(Layout::parse(doc(R"({"code": "KeyQ", "normal": "йй", "finger": "left_pinky"})")), LoadError);
    CHECK_THROWS_AS(Layout::parse(doc(R"({"code": "KeyQ", "normal": "й", "shift": "Й", "finger": "left_pinky", "shift_with": "Nope"})")), LoadError);
    CHECK_THROWS_AS(Layout::parse(R"({"format": "other", "version": 1, "rows": []})"), LoadError);
    CHECK_THROWS_AS(Layout::parse("{"), LoadError);
    CHECK_THROWS_AS(Layout::load(KBSIM_DATA_DIR "/layout/missing.json"), LoadError);
}
