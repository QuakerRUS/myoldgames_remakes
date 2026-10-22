#include "piton/level.hpp"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <fstream>
#include <sstream>

namespace piton {

namespace {

// ---- A minimal JSON reader, enough for the level format. ----

struct JValue {
    enum class Kind { Null, Bool, Number, String, Array, Object } kind = Kind::Null;
    bool b = false;
    double num = 0;
    std::string str;
    std::vector<JValue> arr;
    std::vector<std::pair<std::string, JValue>> obj;

    const JValue* get(std::string_view key) const {
        for (const auto& [k, v] : obj)
            if (k == key) return &v;
        return nullptr;
    }
};

class JParser {
public:
    explicit JParser(std::string_view s) : s_(s) {}

    bool parse(JValue& out, std::string& err) {
        skipWs();
        if (!value(out, 0)) { err = err_; return false; }
        skipWs();
        if (pos_ != s_.size()) { err = at("trailing characters"); return false; }
        return true;
    }

private:
    std::string_view s_;
    size_t pos_ = 0;
    std::string err_;

    std::string at(const char* msg) const {
        int line = 1;
        for (size_t i = 0; i < pos_ && i < s_.size(); ++i)
            if (s_[i] == '\n') ++line;
        return "JSON line " + std::to_string(line) + ": " + msg;
    }
    bool fail(const char* msg) { err_ = at(msg); return false; }
    void skipWs() {
        while (pos_ < s_.size() && (s_[pos_] == ' ' || s_[pos_] == '\t' || s_[pos_] == '\n' || s_[pos_] == '\r'))
            ++pos_;
    }
    bool lit(std::string_view w) {
        if (s_.substr(pos_, w.size()) != w) return false;
        pos_ += w.size();
        return true;
    }

    bool value(JValue& v, int depth) {
        if (depth > 32) return fail("nesting too deep");
        if (pos_ >= s_.size()) return fail("unexpected end");
        char c = s_[pos_];
        if (c == '{') return object(v, depth);
        if (c == '[') return array(v, depth);
        if (c == '"') { v.kind = JValue::Kind::String; return string(v.str); }
        if (lit("true")) { v.kind = JValue::Kind::Bool; v.b = true; return true; }
        if (lit("false")) { v.kind = JValue::Kind::Bool; v.b = false; return true; }
        if (lit("null")) { v.kind = JValue::Kind::Null; return true; }
        if (c == '-' || (c >= '0' && c <= '9')) return number(v);
        return fail("unexpected character");
    }

    bool number(JValue& v) {
        size_t start = pos_;
        if (s_[pos_] == '-') ++pos_;
        while (pos_ < s_.size() && ((s_[pos_] >= '0' && s_[pos_] <= '9') || s_[pos_] == '.' ||
                                    s_[pos_] == 'e' || s_[pos_] == 'E' || s_[pos_] == '+' || s_[pos_] == '-'))
            ++pos_;
        std::string tok(s_.substr(start, pos_ - start));
        char* end = nullptr;
        v.num = std::strtod(tok.c_str(), &end);
        if (end == tok.c_str() || *end != '\0') return fail("bad number");
        v.kind = JValue::Kind::Number;
        return true;
    }

    bool string(std::string& out) {
        ++pos_;  // opening quote
        out.clear();
        while (pos_ < s_.size()) {
            char c = s_[pos_++];
            if (c == '"') return true;
            if (c == '\\') {
                if (pos_ >= s_.size()) break;
                char e = s_[pos_++];
                switch (e) {
                case '"': out += '"'; break;
                case '\\': out += '\\'; break;
                case '/': out += '/'; break;
                case 'b': out += '\b'; break;
                case 'f': out += '\f'; break;
                case 'n': out += '\n'; break;
                case 'r': out += '\r'; break;
                case 't': out += '\t'; break;
                case 'u': {
                    if (pos_ + 4 > s_.size()) return fail("bad \\u escape");
                    unsigned cp = 0;
                    for (int i = 0; i < 4; ++i) {
                        char h = s_[pos_++];
                        cp <<= 4;
                        if (h >= '0' && h <= '9') cp |= unsigned(h - '0');
                        else if (h >= 'a' && h <= 'f') cp |= unsigned(h - 'a' + 10);
                        else if (h >= 'A' && h <= 'F') cp |= unsigned(h - 'A' + 10);
                        else return fail("bad \\u escape");
                    }
                    // UTF-8 encode (surrogate pairs are not combined; not needed here).
                    if (cp < 0x80) out += char(cp);
                    else if (cp < 0x800) { out += char(0xC0 | (cp >> 6)); out += char(0x80 | (cp & 0x3F)); }
                    else { out += char(0xE0 | (cp >> 12)); out += char(0x80 | ((cp >> 6) & 0x3F)); out += char(0x80 | (cp & 0x3F)); }
                    break;
                }
                default: return fail("bad escape");
                }
            } else {
                out += c;
            }
        }
        return fail("unterminated string");
    }

    bool array(JValue& v, int depth) {
        v.kind = JValue::Kind::Array;
        ++pos_;
        skipWs();
        if (pos_ < s_.size() && s_[pos_] == ']') { ++pos_; return true; }
        for (;;) {
            skipWs();
            v.arr.emplace_back();
            if (!value(v.arr.back(), depth + 1)) return false;
            skipWs();
            if (pos_ < s_.size() && s_[pos_] == ',') { ++pos_; continue; }
            if (pos_ < s_.size() && s_[pos_] == ']') { ++pos_; return true; }
            return fail("expected ',' or ']'");
        }
    }

    bool object(JValue& v, int depth) {
        v.kind = JValue::Kind::Object;
        ++pos_;
        skipWs();
        if (pos_ < s_.size() && s_[pos_] == '}') { ++pos_; return true; }
        for (;;) {
            skipWs();
            if (pos_ >= s_.size() || s_[pos_] != '"') return fail("expected key");
            std::string key;
            if (!string(key)) return false;
            skipWs();
            if (pos_ >= s_.size() || s_[pos_] != ':') return fail("expected ':'");
            ++pos_;
            skipWs();
            JValue item;
            if (!value(item, depth + 1)) return false;
            v.obj.emplace_back(std::move(key), std::move(item));
            skipWs();
            if (pos_ < s_.size() && s_[pos_] == ',') { ++pos_; continue; }
            if (pos_ < s_.size() && s_[pos_] == '}') { ++pos_; return true; }
            return fail("expected ',' or '}'");
        }
    }
};

std::string jsonEscape(std::string_view s) {
    std::string out;
    for (char c : s) {
        switch (c) {
        case '"': out += "\\\""; break;
        case '\\': out += "\\\\"; break;
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        default:
            if (static_cast<unsigned char>(c) < 0x20) {
                char buf[8];
                std::snprintf(buf, sizeof buf, "\\u%04x", unsigned(static_cast<unsigned char>(c)));
                out += buf;
            } else {
                out += c;
            }
        }
    }
    return out;
}

bool setErr(std::string* error, std::string msg) {
    if (error) *error = std::move(msg);
    return false;
}

bool getInt(const JValue& v, int& out) {
    if (v.kind != JValue::Kind::Number) return false;
    int i = static_cast<int>(v.num);
    if (static_cast<double>(i) != v.num) return false;
    out = i;
    return true;
}

} // namespace

Level::Level() { tiles_.fill(Tile::Empty); }

Tile Level::tile(Cell c) const {
    if (!insideField(c)) return Tile::Brick;
    return tiles_[static_cast<size_t>(cellIndex(c))];
}

void Level::setTile(Cell c, Tile t) {
    if (insideField(c)) tiles_[static_cast<size_t>(cellIndex(c))] = t;
}

void Level::fill(Tile t) { tiles_.fill(t); }

std::array<Cell, kStartLength> Level::startCellsFor(Cell head, Dir dir) {
    std::array<Cell, kStartLength> cells{};
    Cell c = head;
    const Dir back = opposite(dir);
    for (int i = 0; i < kStartLength; ++i) {
        cells[static_cast<size_t>(i)] = c;
        c = step(c, back);
    }
    return cells;
}

bool Level::startFits(Cell head, Dir dir) {
    for (Cell c : startCellsFor(head, dir))
        if (!insideField(c)) return false;
    return true;
}

Cell Level::clampStart(Cell head, Dir dir) {
    // The body extends opposite to dir by kStartLength-1 cells.
    constexpr int body = kStartLength - 1;
    int minX = 0, maxX = kFieldWidth - 1, minY = 0, maxY = kFieldHeight - 1;
    switch (dir) {
    case Dir::Up: maxY -= body; break;
    case Dir::Down: minY += body; break;
    case Dir::Left: maxX -= body; break;
    case Dir::Right: minX += body; break;
    }
    return {std::clamp(head.x, minX, maxX), std::clamp(head.y, minY, maxY)};
}

bool Level::setStart(Cell head, Dir dir) {
    if (!startFits(head, dir)) return false;
    startHead_ = head;
    startDir_ = dir;
    for (Cell c : startCells()) setTile(c, Tile::Empty);
    return true;
}

int Level::freeCellCount() const {
    int n = 0;
    for (Tile t : tiles_)
        if (t == Tile::Empty) ++n;
    return n;
}

std::vector<std::string> Level::rows() const {
    std::vector<std::string> out;
    for (int y = 0; y < kFieldHeight; ++y) {
        std::string row;
        for (int x = 0; x < kFieldWidth; ++x) row += isBrick({x, y}) ? '#' : '.';
        out.push_back(std::move(row));
    }
    return out;
}

bool Level::fromJson(std::string_view json, Level& out, std::string* error) {
    JValue root;
    std::string err;
    if (!JParser(json).parse(root, err)) return setErr(error, err);
    if (root.kind != JValue::Kind::Object) return setErr(error, "root is not an object");

    const JValue* fmt = root.get("format");
    if (!fmt || fmt->kind != JValue::Kind::String || fmt->str != "piton-level")
        return setErr(error, "\"format\" must be \"piton-level\"");
    int version = 0;
    const JValue* ver = root.get("version");
    if (!ver || !getInt(*ver, version) || version != 1)
        return setErr(error, "unsupported \"version\" (expected 1)");

    int w = 0, h = 0;
    const JValue* jw = root.get("width");
    const JValue* jh = root.get("height");
    if (!jw || !getInt(*jw, w) || !jh || !getInt(*jh, h) || w != kFieldWidth || h != kFieldHeight)
        return setErr(error, "\"width\"/\"height\" must be 38 and 23");

    Level lv;
    if (const JValue* name = root.get("name")) {
        if (name->kind != JValue::Kind::String) return setErr(error, "\"name\" must be a string");
        lv.name = name->str;
    }

    const JValue* rows = root.get("rows");
    if (!rows || rows->kind != JValue::Kind::Array || static_cast<int>(rows->arr.size()) != kFieldHeight)
        return setErr(error, "\"rows\" must be an array of 23 strings");
    for (int y = 0; y < kFieldHeight; ++y) {
        const JValue& r = rows->arr[static_cast<size_t>(y)];
        if (r.kind != JValue::Kind::String || static_cast<int>(r.str.size()) != kFieldWidth)
            return setErr(error, "row " + std::to_string(y) + " must be a string of 38 characters");
        for (int x = 0; x < kFieldWidth; ++x) {
            char c = r.str[static_cast<size_t>(x)];
            if (c == '#') lv.setTile({x, y}, Tile::Brick);
            else if (c == '.') lv.setTile({x, y}, Tile::Empty);
            else
                return setErr(error, "row " + std::to_string(y) + " column " + std::to_string(x) +
                                         ": unknown character (use '#' or '.')");
        }
    }

    const JValue* start = root.get("start");
    if (!start || start->kind != JValue::Kind::Object) return setErr(error, "\"start\" object missing");
    Cell head;
    const JValue* sx = start->get("x");
    const JValue* sy = start->get("y");
    const JValue* sd = start->get("dir");
    if (!sx || !getInt(*sx, head.x) || !sy || !getInt(*sy, head.y))
        return setErr(error, "\"start\" needs integer \"x\" and \"y\"");
    if (!insideField(head)) return setErr(error, "start is outside the field");
    if (!sd || sd->kind != JValue::Kind::String || !dirFromName(sd->str))
        return setErr(error, "\"start.dir\" must be \"up\", \"right\", \"down\" or \"left\"");
    if (!lv.setStart(head, *dirFromName(sd->str)))  // also clears bricks under the start
        return setErr(error, "the start snake at (" + std::to_string(head.x) + "," + std::to_string(head.y) +
                                 ") facing " + sd->str + " does not fit inside the field");

    out = std::move(lv);
    return true;
}

std::string Level::toJson() const {
    std::ostringstream o;
    o << "{\n";
    o << "  \"format\": \"piton-level\",\n";
    o << "  \"version\": 1,\n";
    o << "  \"name\": \"" << jsonEscape(name) << "\",\n";
    o << "  \"width\": " << kFieldWidth << ",\n";
    o << "  \"height\": " << kFieldHeight << ",\n";
    o << "  \"start\": {\n";
    o << "    \"x\": " << startHead_.x << ",\n";
    o << "    \"y\": " << startHead_.y << ",\n";
    o << "    \"dir\": \"" << dirName(startDir_) << "\"\n";
    o << "  },\n";
    o << "  \"rows\": [\n";
    auto r = rows();
    for (size_t i = 0; i < r.size(); ++i)
        o << "    \"" << r[i] << "\"" << (i + 1 < r.size() ? ",\n" : "\n");
    o << "  ]\n";
    o << "}\n";
    return o.str();
}

bool Level::load(const std::filesystem::path& file, Level& out, std::string* error) {
    std::ifstream in(file, std::ios::binary);
    if (!in) return setErr(error, "cannot open " + file.string());
    std::ostringstream ss;
    ss << in.rdbuf();
    std::string text = ss.str();
    if (text.size() >= 3 && text.compare(0, 3, "\xEF\xBB\xBF") == 0) text.erase(0, 3);  // UTF-8 BOM
    std::string err;
    if (!fromJson(text, out, &err)) return setErr(error, file.string() + ": " + err);
    return true;
}

bool Level::save(const std::filesystem::path& file, std::string* error) const {
    std::ofstream out(file, std::ios::binary | std::ios::trunc);
    if (!out) return setErr(error, "cannot write " + file.string());
    out << toJson();
    if (!out) return setErr(error, "write failed: " + file.string());
    return true;
}

LevelCheck checkLevel(const Level& level) {
    LevelCheck r;
    r.freeCells = level.freeCellCount();

    auto error = [&r](std::string msg) {
        r.errors.push_back(msg);
        r.problems.push_back(std::move(msg));
    };
    auto warning = [&r](std::string msg) {
        r.warnings.push_back(msg);
        r.problems.push_back(std::move(msg));
    };

    // Start cells.
    bool startOk = true;
    for (Cell c : level.startCells()) {
        const std::string at = "(" + std::to_string(c.x) + "," + std::to_string(c.y) + ")";
        if (!insideField(c)) {
            error("start segment " + at + " is outside the field");
            startOk = false;
        } else if (!level.isFree(c)) {
            error("start segment " + at + " is not a free cell");
            startOk = false;
        }
    }
    if (startOk) {
        Cell c = level.startHead();
        for (int i = 0; i < kMinFreeAhead; ++i) {
            c = step(c, level.startDir());
            if (!level.isFree(c)) {
                warning("fewer than " + std::to_string(kMinFreeAhead) + " free cells ahead of the start");
                break;
            }
        }
        if (r.freeCells <= kStartLength) error("no free cell for a rabbit");
    }

    // Connectivity.
    std::array<int, kCellCount> region{};
    region.fill(-1);
    for (int i = 0; i < kCellCount; ++i) {
        if (region[static_cast<size_t>(i)] >= 0 || !level.isFree(cellAt(i))) continue;
        int id = r.regions++;
        std::deque<int> q{i};
        region[static_cast<size_t>(i)] = id;
        while (!q.empty()) {
            Cell c = cellAt(q.front());
            q.pop_front();
            for (Dir d : kAllDirs) {
                Cell n = step(c, d);
                if (level.isFree(n) && region[static_cast<size_t>(cellIndex(n))] < 0) {
                    region[static_cast<size_t>(cellIndex(n))] = id;
                    q.push_back(cellIndex(n));
                }
            }
        }
    }
    if (r.regions != 1) error("empty cells form " + std::to_string(r.regions) + " regions (must be 1)");
    if (r.freeCells < kMinFreeCells)
        warning("only " + std::to_string(r.freeCells) + " free cells (need " + std::to_string(kMinFreeCells) + ")");

    // Dead ends: repeatedly peel cells with at most one free neighbour; the number of
    // peeling rounds is the length of the longest 1-wide dead-end corridor.
    std::array<int, kCellCount> deg{};
    std::array<bool, kCellCount> removed{};
    std::vector<int> frontier;
    for (int i = 0; i < kCellCount; ++i) {
        if (!level.isFree(cellAt(i))) continue;
        int n = 0;
        for (Dir d : kAllDirs) n += level.isFree(step(cellAt(i), d)) ? 1 : 0;
        deg[static_cast<size_t>(i)] = n;
        if (n <= 1) frontier.push_back(i);
    }
    int round = 0;
    while (!frontier.empty()) {
        ++round;
        std::vector<int> next;
        for (int i : frontier) {
            if (removed[static_cast<size_t>(i)]) continue;
            removed[static_cast<size_t>(i)] = true;
            r.longestDeadEnd = std::max(r.longestDeadEnd, round);
            for (Dir d : kAllDirs) {
                Cell n = step(cellAt(i), d);
                if (!level.isFree(n)) continue;
                int ni = cellIndex(n);
                if (removed[static_cast<size_t>(ni)]) continue;
                if (--deg[static_cast<size_t>(ni)] <= 1) next.push_back(ni);
            }
        }
        frontier = std::move(next);
        if (round > kCellCount) break;
    }
    if (r.longestDeadEnd > kMaxDeadEndLength)
        warning("a 1-cell-wide dead end is " + std::to_string(r.longestDeadEnd) + " cells long (max " +
                std::to_string(kMaxDeadEndLength) + ")");
    return r;
}

std::string levelFileName(int number) {
    char buf[16];
    std::snprintf(buf, sizeof buf, "%02d.json", number);
    return buf;
}

} // namespace piton
