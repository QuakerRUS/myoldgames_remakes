#include "json.hpp"

#include <charconv>
#include <stdexcept>

#include <kbsim/utf8.hpp>

namespace kbsim::json {

namespace {

[[noreturn]] void type_error(const char* want) {
    throw std::runtime_error(std::string("JSON: expected ") + want);
}

class Parser {
public:
    explicit Parser(std::string_view text) : s_(text) {}

    Value document() {
        Value v = value(0);
        skip_ws();
        if (i_ != s_.size()) fail("trailing characters");
        return v;
    }

private:
    std::string_view s_;
    std::size_t i_ = 0;

    [[noreturn]] void fail(const std::string& what) const {
        throw std::runtime_error("JSON: " + what + " at offset " + std::to_string(i_));
    }

    void skip_ws() {
        while (i_ < s_.size() && (s_[i_] == ' ' || s_[i_] == '\t' || s_[i_] == '\n' || s_[i_] == '\r')) ++i_;
    }

    char peek() {
        skip_ws();
        if (i_ >= s_.size()) fail("unexpected end");
        return s_[i_];
    }

    void expect(char c) {
        if (peek() != c) fail(std::string("expected '") + c + "'");
        ++i_;
    }

    bool literal(std::string_view word) {
        if (s_.substr(i_, word.size()) == word) {
            i_ += word.size();
            return true;
        }
        return false;
    }

    Value value(int depth) {
        if (depth > 64) fail("nesting too deep");
        const char c = peek();
        if (c == '{') return Value{object(depth)};
        if (c == '[') return Value{array(depth)};
        if (c == '"') return Value{string()};
        if (literal("true")) return Value{true};
        if (literal("false")) return Value{false};
        if (literal("null")) return Value{nullptr};
        return Value{number()};
    }

    Object object(int depth) {
        expect('{');
        Object out;
        if (peek() == '}') {
            ++i_;
            return out;
        }
        for (;;) {
            if (peek() != '"') fail("expected a member name");
            std::string key = string();
            expect(':');
            out.push_back(Member{std::move(key), value(depth + 1)});
            if (peek() == ',') {
                ++i_;
                continue;
            }
            expect('}');
            return out;
        }
    }

    Array array(int depth) {
        expect('[');
        Array out;
        if (peek() == ']') {
            ++i_;
            return out;
        }
        for (;;) {
            out.push_back(value(depth + 1));
            if (peek() == ',') {
                ++i_;
                continue;
            }
            expect(']');
            return out;
        }
    }

    unsigned hex4() {
        if (i_ + 4 > s_.size()) fail("truncated \\u escape");
        unsigned v = 0;
        for (int k = 0; k < 4; ++k) {
            const char c = s_[i_++];
            v <<= 4;
            if (c >= '0' && c <= '9') v |= static_cast<unsigned>(c - '0');
            else if (c >= 'a' && c <= 'f') v |= static_cast<unsigned>(c - 'a' + 10);
            else if (c >= 'A' && c <= 'F') v |= static_cast<unsigned>(c - 'A' + 10);
            else fail("bad \\u escape");
        }
        return v;
    }

    std::string string() {
        expect('"');
        std::string out;
        for (;;) {
            if (i_ >= s_.size()) fail("unterminated string");
            const char c = s_[i_++];
            if (c == '"') return out;
            if (static_cast<unsigned char>(c) < 0x20) fail("control character in string");
            if (c != '\\') {
                out.push_back(c);
                continue;
            }
            if (i_ >= s_.size()) fail("unterminated escape");
            const char e = s_[i_++];
            switch (e) {
                case '"': out.push_back('"'); break;
                case '\\': out.push_back('\\'); break;
                case '/': out.push_back('/'); break;
                case 'b': out.push_back('\b'); break;
                case 'f': out.push_back('\f'); break;
                case 'n': out.push_back('\n'); break;
                case 'r': out.push_back('\r'); break;
                case 't': out.push_back('\t'); break;
                case 'u': {
                    char32_t cp = hex4();
                    if (cp >= 0xD800 && cp <= 0xDBFF) {
                        if (!literal("\\u")) fail("lone surrogate");
                        const char32_t lo = hex4();
                        if (lo < 0xDC00 || lo > 0xDFFF) fail("bad surrogate pair");
                        cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00);
                    } else if (cp >= 0xDC00 && cp <= 0xDFFF) {
                        fail("lone surrogate");
                    }
                    out += u32_to_utf8(cp);
                    break;
                }
                default: fail("bad escape");
            }
        }
    }

    double number() {
        skip_ws();
        const std::size_t start = i_;
        while (i_ < s_.size() && (std::string_view("+-0123456789.eE").find(s_[i_]) != std::string_view::npos)) ++i_;
        if (start == i_) fail("unexpected character");
        double d = 0;
        const auto [ptr, ec] = std::from_chars(s_.data() + start, s_.data() + i_, d);
        if (ec != std::errc{} || ptr != s_.data() + i_) {
            i_ = start;
            fail("bad number");
        }
        return d;
    }
};

}  // namespace

bool Value::as_bool() const {
    if (!is_bool()) type_error("a boolean");
    return std::get<bool>(v);
}
double Value::as_number() const {
    if (!is_number()) type_error("a number");
    return std::get<double>(v);
}
const std::string& Value::as_string() const {
    if (!is_string()) type_error("a string");
    return std::get<std::string>(v);
}
const Array& Value::as_array() const {
    if (!is_array()) type_error("an array");
    return std::get<Array>(v);
}
const Object& Value::as_object() const {
    if (!is_object()) type_error("an object");
    return std::get<Object>(v);
}
const Value* Value::find(std::string_view key) const {
    for (const Member& m : as_object()) {
        if (m.key == key) return &m.value;
    }
    return nullptr;
}

Value parse(std::string_view text) {
    if (text.starts_with("\xEF\xBB\xBF")) text.remove_prefix(3);
    return Parser(text).document();
}

}  // namespace kbsim::json
