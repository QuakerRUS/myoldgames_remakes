#pragma once
// Minimal JSON reader used to load the layout file (private to kbsim_core).

#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace kbsim::json {

struct Value;
struct Member;
using Array = std::vector<Value>;
using Object = std::vector<Member>;  // keeps the order of the file

struct Value {
    std::variant<std::nullptr_t, bool, double, std::string, Array, Object> v;

    bool is_null() const { return std::holds_alternative<std::nullptr_t>(v); }
    bool is_bool() const { return std::holds_alternative<bool>(v); }
    bool is_number() const { return std::holds_alternative<double>(v); }
    bool is_string() const { return std::holds_alternative<std::string>(v); }
    bool is_array() const { return std::holds_alternative<Array>(v); }
    bool is_object() const { return std::holds_alternative<Object>(v); }

    // Typed accessors; throw std::runtime_error on a type mismatch.
    bool as_bool() const;
    double as_number() const;
    const std::string& as_string() const;
    const Array& as_array() const;
    const Object& as_object() const;

    // Member lookup in an object; nullptr if absent (throws if this is not an object).
    const Value* find(std::string_view key) const;
};

struct Member {
    std::string key;
    Value value;
};

// Parses a complete JSON document. Throws std::runtime_error with the offset on errors.
Value parse(std::string_view text);

}  // namespace kbsim::json
