#pragma once
// The on-screen keyboard: keys, their characters and the finger that presses each one.
// File format: docs/formats/kbsim-layout.md.

#include <array>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace kbsim {

// Fingers in the order of the finger legend "5 4 3 2 1 / 1 2 3 4 5" (the two thumbs are one
// entry: the space bar may be pressed by either thumb).
enum class Finger : std::uint8_t {
    LeftPinky,
    LeftRing,
    LeftMiddle,
    LeftIndex,
    Thumbs,
    RightIndex,
    RightMiddle,
    RightRing,
    RightPinky,
};
inline constexpr int kFingerCount = 9;

enum class Hand : std::uint8_t { Left, Right, Both };

Hand hand_of(Finger finger);
// The digit of the finger legend: pinky 5, ring 4, middle 3, index 2, thumb 1.
int legend_number(Finger finger);
// "left_pinky", ..., "thumbs", ..., "right_pinky" (the ids used in the layout file).
std::string_view finger_id(Finger finger);
std::optional<Finger> finger_from_id(std::string_view id);

using KeyIndex = int;  // index into Layout::keys()
inline constexpr KeyIndex kNoKey = -1;

struct Key {
    std::string code;        // physical key, W3C KeyboardEvent.code name: "KeyQ", "ShiftLeft", ...
    char32_t normal = 0;     // character without Shift, 0 if none (modifier keys)
    char32_t shifted = 0;    // character with Shift, 0 if none
    std::string label;       // UTF-8 caption; empty for symbol keys (draw normal + shifted) and Space
    int row = 0;             // 0 = digit row ... 4 = Ctrl/Win/Alt/Space row
    float x = 0;             // left edge in key units from the left edge of the keyboard
    float width = 1;         // in key units (a letter key is 1 x 1)
    Finger finger = Finger::LeftPinky;
    bool home = false;       // home-row key (ФЫВА ОЛДЖ)
    bool bump = false;       // key with a tactile bump (А, О)
    KeyIndex shift_with = kNoKey;  // the Shift to hold for `shifted` (the opposite hand's)
};

// How to type one character.
struct Stroke {
    char32_t ch = 0;
    KeyIndex key = kNoKey;        // the key carrying the character
    bool shift = false;           // true if Shift must be held
    KeyIndex shift_key = kNoKey;  // ShiftLeft or ShiftRight when shift, else kNoKey
    Finger finger = Finger::LeftPinky;  // the finger for `key`
};

class Layout {
public:
    // Throws LoadError on a missing or malformed file.
    static Layout load(const std::filesystem::path& file);
    // Parses the JSON text. Throws LoadError on malformed content (unknown finger, a character
    // on two keys, a shifted character without "shift_with", ...).
    static Layout parse(std::string_view json_text);

    const std::string& name() const { return name_; }
    const std::vector<Key>& keys() const { return keys_; }
    const Key& key(KeyIndex index) const { return keys_.at(static_cast<std::size_t>(index)); }
    int row_count() const { return static_cast<int>(rows_.size()); }
    // Key indices of one row, left to right.
    const std::vector<KeyIndex>& row(int r) const { return rows_.at(static_cast<std::size_t>(r)); }
    // Total width of a row and of the widest row, in key units.
    float row_width(int r) const;
    float width() const;

    // kNoKey if there is no key with that code.
    KeyIndex find_code(std::string_view code) const;
    // How to type `ch`; nullopt if the layout has no such character.
    std::optional<Stroke> stroke(char32_t ch) const;
    bool can_type(char32_t ch) const { return stroke(ch).has_value(); }

private:
    std::string name_;
    std::vector<Key> keys_;
    std::vector<std::vector<KeyIndex>> rows_;
    std::unordered_map<char32_t, Stroke> strokes_;
};

}  // namespace kbsim
