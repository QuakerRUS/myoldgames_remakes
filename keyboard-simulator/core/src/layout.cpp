#include <kbsim/layout.hpp>

#include <algorithm>
#include <stdexcept>

#include <kbsim/utf8.hpp>

#include "json.hpp"

namespace kbsim {

namespace {

constexpr std::array<std::string_view, kFingerCount> kFingerIds = {
    "left_pinky", "left_ring", "left_middle", "left_index", "thumbs",
    "right_index", "right_middle", "right_ring", "right_pinky",
};

// A string member holding exactly one character.
char32_t single_char(const json::Value& v, const std::string& where) {
    const std::u32string s = utf8_to_u32(v.as_string());
    if (s.size() != 1) throw LoadError("layout: " + where + " must be exactly one character");
    return s[0];
}

}  // namespace

Hand hand_of(Finger finger) {
    if (finger == Finger::Thumbs) return Hand::Both;
    return finger < Finger::Thumbs ? Hand::Left : Hand::Right;
}

int legend_number(Finger finger) {
    static constexpr std::array<int, kFingerCount> numbers = {5, 4, 3, 2, 1, 2, 3, 4, 5};
    return numbers[static_cast<std::size_t>(finger)];
}

std::string_view finger_id(Finger finger) { return kFingerIds[static_cast<std::size_t>(finger)]; }

std::optional<Finger> finger_from_id(std::string_view id) {
    for (std::size_t i = 0; i < kFingerIds.size(); ++i) {
        if (kFingerIds[i] == id) return static_cast<Finger>(i);
    }
    return std::nullopt;
}

Layout Layout::load(const std::filesystem::path& file) {
    try {
        return parse(read_file(file));
    } catch (const LoadError& e) {
        throw LoadError(file.string() + ": " + e.what());
    }
}

Layout Layout::parse(std::string_view json_text) {
    Layout layout;
    std::vector<std::string> shift_codes;  // "shift_with" of each key, resolved after all keys
    try {
        const json::Value doc = json::parse(json_text);
        const json::Value* format = doc.find("format");
        if (!format || format->as_string() != "kbsim-layout") throw LoadError("layout: format is not kbsim-layout");
        const json::Value* version = doc.find("version");
        if (!version || version->as_number() != 1) throw LoadError("layout: unsupported version");
        if (const json::Value* name = doc.find("name")) layout.name_ = name->as_string();
        const json::Value* rows = doc.find("rows");
        if (!rows) throw LoadError("layout: no rows");

        int r = 0;
        for (const json::Value& row : rows->as_array()) {
            std::vector<KeyIndex> indices;
            float x = 0;
            for (const json::Value& k : row.as_array()) {
                Key key;
                const json::Value* code = k.find("code");
                if (!code) throw LoadError("layout: key without code in row " + std::to_string(r));
                key.code = code->as_string();
                const std::string where = "key " + key.code;
                if (layout.find_code(key.code) != kNoKey) throw LoadError("layout: duplicate " + where);
                if (const json::Value* v = k.find("normal")) key.normal = single_char(*v, where + " normal");
                if (const json::Value* v = k.find("shift")) key.shifted = single_char(*v, where + " shift");
                if (const json::Value* v = k.find("label")) key.label = v->as_string();
                if (const json::Value* v = k.find("width")) key.width = static_cast<float>(v->as_number());
                if (key.width <= 0) throw LoadError("layout: " + where + " has a bad width");
                const json::Value* finger = k.find("finger");
                if (!finger) throw LoadError("layout: " + where + " has no finger");
                const auto f = finger_from_id(finger->as_string());
                if (!f) throw LoadError("layout: " + where + " has an unknown finger " + finger->as_string());
                key.finger = *f;
                if (const json::Value* v = k.find("home")) key.home = v->as_bool();
                if (const json::Value* v = k.find("bump")) key.bump = v->as_bool();
                const json::Value* sw = k.find("shift_with");
                if (key.shifted != 0 && !sw) throw LoadError("layout: " + where + " has no shift_with");
                shift_codes.push_back(sw ? sw->as_string() : std::string());
                key.row = r;
                key.x = x;
                x += key.width;
                indices.push_back(static_cast<KeyIndex>(layout.keys_.size()));
                layout.keys_.push_back(std::move(key));
            }
            layout.rows_.push_back(std::move(indices));
            ++r;
        }
    } catch (const std::runtime_error& e) {  // json errors, type mismatches, bad UTF-8
        if (dynamic_cast<const LoadError*>(&e)) throw;
        throw LoadError(std::string("layout: ") + e.what());
    } catch (const std::invalid_argument& e) {
        throw LoadError(std::string("layout: ") + e.what());
    }

    for (std::size_t i = 0; i < layout.keys_.size(); ++i) {
        Key& key = layout.keys_[i];
        if (!shift_codes[i].empty()) {
            key.shift_with = layout.find_code(shift_codes[i]);
            if (key.shift_with == kNoKey) throw LoadError("layout: key " + key.code + ": unknown shift_with " + shift_codes[i]);
        }
        const auto add = [&](char32_t ch, bool shift) {
            if (ch == 0) return;
            Stroke s{ch, static_cast<KeyIndex>(i), shift, shift ? key.shift_with : kNoKey, key.finger};
            if (!layout.strokes_.emplace(ch, s).second) {
                throw LoadError("layout: character U+" + std::to_string(static_cast<std::uint32_t>(ch)) +
                                " is on two keys");
            }
        };
        add(key.normal, false);
        add(key.shifted, true);
    }
    return layout;
}

float Layout::row_width(int r) const {
    float w = 0;
    for (KeyIndex k : row(r)) w += key(k).width;
    return w;
}

float Layout::width() const {
    float w = 0;
    for (int r = 0; r < row_count(); ++r) w = std::max(w, row_width(r));
    return w;
}

KeyIndex Layout::find_code(std::string_view code) const {
    for (std::size_t i = 0; i < keys_.size(); ++i) {
        if (keys_[i].code == code) return static_cast<KeyIndex>(i);
    }
    return kNoKey;
}

std::optional<Stroke> Layout::stroke(char32_t ch) const {
    const auto it = strokes_.find(ch);
    if (it == strokes_.end()) return std::nullopt;
    return it->second;
}

}  // namespace kbsim
