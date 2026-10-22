#include "level_doc.hpp"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <sstream>

#include <vs/engine/error.hpp>
#include <vs/engine/json_file.hpp>
#include <vs/engine/storage.hpp>

namespace piton_editor {

using piton::Cell;
using piton::Dir;
using piton::kFieldHeight;
using piton::kFieldWidth;

namespace {

std::string readBytes(const std::filesystem::path& file) {
    std::ifstream in(file, std::ios::binary);
    if (!in) throw vs::Error("cannot open " + vs::path_to_utf8(file));
    return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

vs::TileDocument mapCells(const vs::TileDocument& doc, auto&& source) {
    vs::TileDocument out = doc;
    for (int y = 0; y < kFieldHeight; ++y)
        for (int x = 0; x < kFieldWidth; ++x) {
            const Cell s = source(x, y);
            out.grid.set(x, y, doc.grid.at(s.x, s.y));
        }
    return out;
}

// Removes bricks from the start snake's cells (after a whole-document tool).
vs::TileDocument keepStartFree(vs::TileDocument doc) {
    for (Cell c : piton::Level::startCellsFor(docStart(doc), docStartDir(doc)))
        if (doc.grid.in_bounds(c.x, c.y)) doc.grid.set(c.x, c.y, kEmpty);
    return doc;
}

bool isStartCell(const vs::TileDocument& doc, Cell c) {
    for (Cell s : piton::Level::startCellsFor(docStart(doc), docStartDir(doc)))
        if (s == c) return true;
    return false;
}

Dir mirrorDirX(Dir d) { return d == Dir::Left ? Dir::Right : d == Dir::Right ? Dir::Left : d; }
Dir mirrorDirY(Dir d) { return d == Dir::Up ? Dir::Down : d == Dir::Down ? Dir::Up : d; }

}  // namespace

std::optional<Dir> startCodeDir(int code) {
    for (Dir d : piton::kAllDirs)
        if (code == startCode(d)) return d;
    return std::nullopt;
}

vs::TileDocument toDocument(const piton::Level& level) {
    vs::TileDocument doc{vs::TileGrid(kFieldWidth, kFieldHeight, kEmpty), nlohmann::json::object()};
    for (int y = 0; y < kFieldHeight; ++y)
        for (int x = 0; x < kFieldWidth; ++x) doc.grid.set(x, y, level.isBrick({x, y}) ? kBrick : kEmpty);
    setDocName(doc, level.name);
    setDocStart(doc, level.startHead(), level.startDir());
    return doc;
}

piton::Level toLevel(const vs::TileDocument& doc) {
    piton::Level level;
    level.name = docName(doc);
    // The start first (it clears its cells), then the grid: a brick under the start stays
    // visible to piton::checkLevel.
    const Dir dir = docStartDir(doc);
    level.setStart(piton::Level::clampStart(docStart(doc), dir), dir);
    for (int y = 0; y < kFieldHeight; ++y)
        for (int x = 0; x < kFieldWidth; ++x) {
            const bool brick = doc.grid.in_bounds(x, y) && doc.grid.at(x, y) == kBrick;
            level.setTile({x, y}, brick ? piton::Tile::Brick : piton::Tile::Empty);
        }
    return level;
}

vs::TileDocument newDocument() {
    piton::Level level;
    level.fill(piton::Tile::Empty);
    level.setStart(kNewLevelStart, Dir::Right);
    return toDocument(level);
}

std::string docName(const vs::TileDocument& doc) {
    const auto it = doc.properties.find("name");
    return it != doc.properties.end() && it->is_string() ? it->get<std::string>() : std::string();
}

Cell docStart(const vs::TileDocument& doc) {
    const auto& p = doc.properties;
    if (!p.contains("start") || !p["start"].is_object()) return kNewLevelStart;
    const auto& s = p["start"];
    return {s.value("x", kNewLevelStart.x), s.value("y", kNewLevelStart.y)};
}

Dir docStartDir(const vs::TileDocument& doc) {
    const auto& p = doc.properties;
    if (!p.contains("start") || !p["start"].is_object()) return Dir::Right;
    return piton::dirFromName(p["start"].value("dir", std::string("right"))).value_or(Dir::Right);
}

void setDocName(vs::TileDocument& doc, const std::string& name) { doc.properties["name"] = name; }

void setDocStart(vs::TileDocument& doc, Cell head, Dir dir) {
    doc.properties["start"] = {{"x", head.x}, {"y", head.y}, {"dir", std::string(piton::dirName(dir))}};
}

const char* symmetryName(Symmetry s) {
    switch (s) {
    case Symmetry::Horizontal: return "Horizontal";
    case Symmetry::Vertical: return "Vertical";
    case Symmetry::Both: return "Both";
    case Symmetry::None: break;
    }
    return "Off";
}

bool mirrorsX(Symmetry s) { return s == Symmetry::Horizontal || s == Symmetry::Both; }
bool mirrorsY(Symmetry s) { return s == Symmetry::Vertical || s == Symmetry::Both; }
Symmetry makeSymmetry(bool x, bool y) {
    return x && y ? Symmetry::Both : x ? Symmetry::Horizontal : y ? Symmetry::Vertical : Symmetry::None;
}

std::vector<Cell> symmetricCells(Cell c, Symmetry s) {
    std::vector<Cell> out{c};
    auto add = [&out](Cell m) {
        if (std::find(out.begin(), out.end(), m) == out.end()) out.push_back(m);
    };
    const int mx = kFieldWidth - 1 - c.x;
    const int my = kFieldHeight - 1 - c.y;
    if (mirrorsX(s)) add({mx, c.y});
    if (mirrorsY(s)) add({c.x, my});
    if (s == Symmetry::Both) add({mx, my});
    return out;
}

std::vector<vs::CellChange> paintChanges(const vs::TileDocument& doc, Cell c, int value, Symmetry s) {
    std::vector<vs::CellChange> out;
    for (Cell m : symmetricCells(c, s))
        if (doc.grid.in_bounds(m.x, m.y) && doc.grid.at(m.x, m.y) != value && !(value == kBrick && isStartCell(doc, m)))
            out.push_back({m.x, m.y, value});
    return out;
}

vs::TileDocument withStart(const vs::TileDocument& doc, Cell head, Dir dir) {
    vs::TileDocument out = doc;
    setDocStart(out, piton::Level::clampStart(head, dir), dir);
    return keepStartFree(std::move(out));
}

vs::TileDocument cleared(const vs::TileDocument& doc) {
    vs::TileDocument out = doc;
    out.grid.fill(kEmpty);
    return out;
}

vs::TileDocument borderFilled(const vs::TileDocument& doc) {
    vs::TileDocument out = doc;
    for (int x = 0; x < kFieldWidth; ++x) {
        out.grid.set(x, 0, kBrick);
        out.grid.set(x, kFieldHeight - 1, kBrick);
    }
    for (int y = 0; y < kFieldHeight; ++y) {
        out.grid.set(0, y, kBrick);
        out.grid.set(kFieldWidth - 1, y, kBrick);
    }
    return keepStartFree(std::move(out));
}

vs::TileDocument flippedX(const vs::TileDocument& doc) {
    vs::TileDocument out = mapCells(doc, [](int x, int y) { return Cell{kFieldWidth - 1 - x, y}; });
    const Cell s = docStart(doc);
    setDocStart(out, {kFieldWidth - 1 - s.x, s.y}, mirrorDirX(docStartDir(doc)));
    return out;
}

vs::TileDocument flippedY(const vs::TileDocument& doc) {
    vs::TileDocument out = mapCells(doc, [](int x, int y) { return Cell{x, kFieldHeight - 1 - y}; });
    const Cell s = docStart(doc);
    setDocStart(out, {s.x, kFieldHeight - 1 - s.y}, mirrorDirY(docStartDir(doc)));
    return out;
}

vs::TileDocument mirroredLeftToRight(const vs::TileDocument& doc) {
    return keepStartFree(mapCells(doc, [](int x, int y) { return Cell{x < kFieldWidth / 2 ? x : kFieldWidth - 1 - x, y}; }));
}

vs::TileDocument mirroredTopToBottom(const vs::TileDocument& doc) {
    return keepStartFree(mapCells(doc, [](int x, int y) { return Cell{x, y <= kFieldHeight / 2 ? y : kFieldHeight - 1 - y}; }));
}

vs::TileDocument loadDocument(const std::filesystem::path& file) {
    const std::string text = readBytes(file);
    piton::Level level;
    std::string error;
    if (!piton::Level::fromJson(text, level, &error))
        throw vs::Error(vs::path_to_utf8(file.filename()) + ": " + error);
    return toDocument(level);
}

void saveDocument(const std::filesystem::path& file, const vs::TileDocument& doc) {
    vs::write_file_atomic(file, toLevel(doc).toJson());
}

std::string suggestFileName(const std::filesystem::path& dir) {
    for (int n = 1; n <= 99; ++n) {
        const std::string name = piton::levelFileName(n);
        std::error_code ec;
        if (!std::filesystem::exists(dir / name, ec)) return name;
    }
    return "level.json";
}

RoundTripReport roundTrip(const std::filesystem::path& dir) {
    RoundTripReport report;
    std::vector<std::filesystem::path> files;
    std::error_code ec;
    for (const auto& e : std::filesystem::directory_iterator(dir, ec))
        if (e.is_regular_file() && e.path().extension() == ".json") files.push_back(e.path());
    if (ec) {
        report.failures = 1;
        report.lines.push_back("cannot list " + vs::path_to_utf8(dir) + ": " + ec.message());
        return report;
    }
    std::sort(files.begin(), files.end());
    const std::filesystem::path tmp = std::filesystem::temp_directory_path() / "piton_editor_roundtrip.json";
    for (const auto& f : files) {
        ++report.files;
        const std::string name = vs::path_to_utf8(f.filename());
        try {
            const std::string original = readBytes(f);
            const vs::TileDocument doc = loadDocument(f);
            saveDocument(tmp, doc);
            const std::string saved = readBytes(tmp);
            const vs::TileDocument again = loadDocument(tmp);
            if (saved != original) {
                std::size_t i = 0;
                while (i < saved.size() && i < original.size() && saved[i] == original[i]) ++i;
                ++report.failures;
                report.lines.push_back("FAIL " + name + ": bytes differ at offset " + std::to_string(i) + " (" +
                                       std::to_string(original.size()) + " -> " + std::to_string(saved.size()) +
                                       " bytes)");
            } else if (!(again == doc)) {
                ++report.failures;
                report.lines.push_back("FAIL " + name + ": document changed after reload");
            } else {
                const piton::LevelCheck check = piton::checkLevel(toLevel(doc));
                report.lines.push_back("ok   " + name + " (" + std::to_string(original.size()) + " bytes, \"" +
                                       docName(doc) + "\", " +
                                       (check.ok() ? std::string("valid") :
                                                     std::to_string(check.problems.size()) + " problem(s)") +
                                       ")");
            }
        } catch (const std::exception& e) {
            ++report.failures;
            report.lines.push_back("FAIL " + name + ": " + e.what());
        }
    }
    std::filesystem::remove(tmp, ec);
    return report;
}

}  // namespace piton_editor
