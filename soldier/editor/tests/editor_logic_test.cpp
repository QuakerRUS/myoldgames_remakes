#include <doctest/doctest.h>

#include <algorithm>
#include <cstdio>
#include <set>
#include <string>

#include "level_check.hpp"
#include "level_io.hpp"
#include "palette.hpp"
#include "placement.hpp"
#include "soldier/tile.hpp"

using namespace soldier_editor;

namespace {

vs::TileDocument doc_from_rows(std::initializer_list<const char*> rows) {
    vs::TileDocument doc{vs::TileGrid(static_cast<int>(std::string(*rows.begin()).size()), static_cast<int>(rows.size()), 0),
                         {{"number", 1}, {"time_limit", 0}}};
    int y = 0;
    for (const char* r : rows) {
        for (int x = 0; r[x]; ++x) doc.grid.set(x, y, static_cast<int>(*soldier::tile_from_char(r[x])));
        ++y;
    }
    return doc;
}

bool contains(const std::vector<std::string>& v, const std::string& part) {
    return std::any_of(v.begin(), v.end(), [&](const std::string& s) { return s.find(part) != std::string::npos; });
}

}  // namespace

TEST_CASE("all 51 shipped levels round-trip byte-identically") {
    const RoundTripResult r = round_trip_directory(std::filesystem::path(SOLDIER_DATA_DIR) / "levels");
    for (const std::string& f : r.failures) MESSAGE(f);
    CHECK(r.files == 51);
    CHECK(r.identical == 51);
}

TEST_CASE("new document is 20x15 with a wall border") {
    const vs::TileDocument d = make_new_document();
    REQUIRE(d.grid.width() == 20);
    REQUIRE(d.grid.height() == 15);
    CHECK(d.grid.at(0, 0) == 1);
    CHECK(d.grid.at(19, 14) == 1);
    CHECK(d.grid.at(10, 0) == 1);
    CHECK(d.grid.at(0, 7) == 1);
    CHECK(d.grid.at(1, 1) == 0);
    CHECK(d.grid.count(1) == 2 * 20 + 2 * 13);
    CHECK(doc_time_limit(d) == 0);
}

TEST_CASE("lenient loading accepts a level without a start, saving keeps the format") {
    const std::string text =
        "{\n  \"number\": 7,\n  \"width\": 3,\n  \"height\": 1,\n  \"time_limit\": 5,\n  \"rows\": [\n    \"#X#\"\n  ]\n}\n";
    const vs::TileDocument d = level_to_document(parse_level_lenient(text));
    CHECK(doc_number(d) == 7);
    CHECK(doc_time_limit(d) == 5);
    CHECK(document_to_json(d) == text);
    CHECK_THROWS(parse_level_lenient("{\"number\":1,\"width\":2,\"height\":1,\"time_limit\":0,\"rows\":[\"#Z\"]}"));
    CHECK_THROWS(parse_level_lenient("{\"number\":1,\"width\":101,\"height\":1,\"time_limit\":0,\"rows\":[]}"));
}

TEST_CASE("palette offers every meaningful tile and no death tiles") {
    std::set<int> codes;
    for (const PaletteEntry& e : flat_palette()) codes.insert(e.code);
    CHECK(codes.size() == flat_palette().size());
    for (int c : {0, 1, 2, 3, 4, 5, 6, 8, 9, 10, 15, 20, 45, 46, 48, 54, 56, 63}) CHECK(codes.count(c) == 1);
    for (int c = 21; c <= 44; ++c) CHECK(codes.count(c) == 1);
    for (int c = 0; c < soldier::kTileCount; ++c) {
        if (soldier::is_death(static_cast<soldier::Tile>(c))) CHECK(codes.count(c) == 0);
        CHECK(!tile_display_name(c).empty());
        CHECK(!tile_hint(c).empty());
    }
}

TEST_CASE("palette navigation wraps") {
    const int n = static_cast<int>(flat_palette().size());
    CHECK(palette_move(0, -1, 0) == n - 1);
    CHECK(palette_move(n - 1, 1, 0) == 0);
    // Up from the first row goes to the last row (keys); down from the last row to the first.
    const auto layout = palette_layout();
    const int up = palette_move(0, 0, -1);
    CHECK(layout[static_cast<std::size_t>(up)].row == layout.back().row);
    CHECK(layout[static_cast<std::size_t>(palette_move(up, 0, 1))].row == 0);
    // Down from "Floor" (row 0) reaches the player starts (row 1).
    CHECK(flat_palette()[static_cast<std::size_t>(palette_move(0, 0, 1))].code == 3);
}

TEST_CASE("check: start and exit errors") {
    CHECK(contains(check_level(doc_from_rows({"#####", "#...#", "#####"})).errors, "No player start"));
    CHECK(contains(check_level(doc_from_rows({"#####", "#v..#", "#####"})).errors, "No exit"));
    const CheckResult two = check_level(doc_from_rows({"#####", "#v^X#", "#####"}));
    CHECK(two.errors.empty());
    CHECK(contains(two.warnings, "1 extra player tile(s) are decorations; the start is the last one in scan order, at (2, 1)"));
    const CheckResult ok = check_level(doc_from_rows({"#####", "#v.X#", "#####"}));
    CHECK(ok.errors.empty());
    CHECK(ok.warnings.empty());
}

TEST_CASE("check: warnings") {
    CHECK(contains(check_level(doc_from_rows({"######", "#v#.X#", "######"})).warnings, "reachable"));
    CHECK(contains(check_level(doc_from_rows({"######", "#v~.X#", "######"})).warnings, "reachable"));
    // Doors and boxes do not block the flood fill.
    CHECK(!contains(check_level(doc_from_rows({"######", "#vDBX#", "##_###"})).warnings, "reachable"));
    CHECK(contains(check_level(doc_from_rows({"######", "#vD.X#", "######"})).warnings, "Doors without buttons"));
    CHECK(!contains(check_level(doc_from_rows({"######", "#vD_X#", "######"})).warnings, "Doors without buttons"));
    CHECK(contains(check_level(doc_from_rows({"######", "#vR.X#", "######"})).warnings, "Red door"));
    CHECK(!contains(check_level(doc_from_rows({"######", "#vRaX#", "######"})).warnings, "Red door"));
}

TEST_CASE("shipped levels: no errors; decorated levels 14 and 27 only warn") {
    for (int n = 1; n <= 51; ++n) {
        char name[8];
        std::snprintf(name, sizeof name, "%02d.json", n);
        const vs::TileDocument d = load_document(std::filesystem::path(SOLDIER_DATA_DIR) / "levels" / name);
        const CheckResult r = check_level(d);
        INFO(name);
        CHECK(r.errors.empty());
        CHECK(contains(r.warnings, "extra player tile") == (n == 14 || n == 27));
    }
}

TEST_CASE("placing a player keeps one start and combines with the ground") {
    vs::TileDocument d = doc_from_rows({"#####", "#v_D#", "#T..#", "#####"});
    auto apply = [&](std::vector<vs::CellChange> ch) {
        for (const auto& c : ch) d.grid.set(c.x, c.y, c.value);
    };
    apply(placement_changes(d, 2, 1, 6));  // right-facing player onto the button
    CHECK(d.grid.at(2, 1) == static_cast<int>(soldier::Tile::PlayerOnButtonRight));
    CHECK(d.grid.at(1, 1) == 0);  // the old start became floor
    apply(placement_changes(d, 3, 1, 3));
    CHECK(d.grid.at(3, 1) == static_cast<int>(soldier::Tile::PlayerInDoorDown));
    CHECK(d.grid.at(2, 1) == static_cast<int>(soldier::Tile::Button));
    apply(placement_changes(d, 1, 2, 4));
    CHECK(d.grid.at(1, 2) == static_cast<int>(soldier::Tile::PlayerInGuillotineUp));
    CHECK(d.grid.at(3, 1) == static_cast<int>(soldier::Tile::Door));
    CHECK(count_starts(d.grid) == 1);
    CHECK(start_placement_warning(d, 1, 2).empty());
    // Other tiles simply replace.
    CHECK(placement_changes(d, 2, 2, 8) == std::vector<vs::CellChange>{{2, 2, 8}});
}

TEST_CASE("placing a start keeps decorative player tiles") {
    vs::TileDocument d = doc_from_rows({"#######", "#>.<.v#", "#.....#", "#######"});  // start = (5, 1)
    auto apply = [&](std::vector<vs::CellChange> ch) {
        for (const auto& c : ch) d.grid.set(c.x, c.y, c.value);
    };
    // New start after the current one: only the current start moves, decorations stay.
    apply(placement_changes(d, 3, 2, 4));
    CHECK(d.grid.at(5, 1) == 0);
    CHECK(d.grid.at(1, 1) == 6);
    CHECK(d.grid.at(3, 1) == 5);
    CHECK(find_start(d.grid) == CellPos{3, 2});
    CHECK(start_placement_warning(d, 3, 2).empty());
    // New tile before a decoration in scan order: nothing is deleted except the moved start,
    // and the editor warns that the game starts at the last player tile.
    apply(placement_changes(d, 2, 1, 3));
    CHECK(d.grid.at(3, 2) == 0);
    CHECK(d.grid.at(1, 1) == 6);
    CHECK(d.grid.at(3, 1) == 5);
    CHECK(d.grid.at(2, 1) == 3);
    CHECK(find_start(d.grid) == CellPos{3, 1});
    CHECK(start_placement_warning(d, 2, 1).find("last player tile in row-major order, at (3, 1)") != std::string::npos);
}

TEST_CASE("resize keeps the top-left content and clamps to 1..100") {
    const vs::TileDocument d = make_new_document();
    const vs::TileDocument big = resized(d, 25, 18);
    CHECK(big.grid.width() == 25);
    CHECK(big.grid.at(0, 0) == 1);
    CHECK(big.grid.at(19, 5) == 1);
    CHECK(big.grid.at(22, 5) == 0);
    const vs::TileDocument huge = resized(d, 500, 0);
    CHECK(huge.grid.width() == 100);
    CHECK(huge.grid.height() == 1);
}

TEST_CASE("level file names follow the number; a mismatch is warned about") {
    CHECK(level_file_name(7) == "07.json");
    CHECK(level_file_name(51) == "51.json");
    CHECK(level_file_name(120) == "120.json");
    CHECK(file_number_warning("levels/07.json", 7).empty());
    CHECK(file_number_warning("levels/my_level.json", 7).empty());  // not a game level name
    CHECK_FALSE(file_number_warning("levels/07.json", 8).empty());
    CHECK(file_number_warning("levels/07.json", 8).find("08.json") != std::string::npos);
}
