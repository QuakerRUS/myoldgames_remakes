#include <doctest/doctest.h>
#include <vs/editor/tile_document.hpp>

#include <memory>

TEST_CASE("tile grid resize keeps the top-left part") {
    vs::TileGrid g(3, 2, 1);
    g.set(2, 1, 7);
    g.set(0, 0, 5);
    g.resize(4, 3, 9);
    CHECK(g.width() == 4);
    CHECK(g.height() == 3);
    CHECK(g.at(0, 0) == 5);
    CHECK(g.at(2, 1) == 7);
    CHECK(g.at(3, 0) == 9);
    CHECK(g.at(0, 2) == 9);
    g.resize(1, 1);
    CHECK(g.at(0, 0) == 5);
    CHECK(g.count(5) == 1);
}

TEST_CASE("cells command undo/redo through the undo stack") {
    vs::TileDocument doc{vs::TileGrid(4, 4, 0), {}};
    vs::UndoStack undo;
    undo.push(std::make_unique<vs::CellsCommand>(doc, std::vector<vs::CellChange>{{1, 1, 3}, {2, 2, 8}}, "Place"));
    CHECK(doc.grid.at(1, 1) == 3);
    CHECK(doc.grid.at(2, 2) == 8);
    undo.undo();
    CHECK(doc.grid.at(1, 1) == 0);
    CHECK(doc.grid.at(2, 2) == 0);
    undo.redo();
    CHECK(doc.grid.at(2, 2) == 8);
}

TEST_CASE("a brush stroke merges into one undo step") {
    vs::TileDocument doc{vs::TileGrid(4, 1, 0), {}};
    vs::UndoStack undo;
    for (int x = 0; x < 4; ++x)
        undo.push(std::make_unique<vs::CellsCommand>(doc, std::vector<vs::CellChange>{{x, 0, 1}}, "Paint", 7), true);
    CHECK(undo.size() == 1);
    CHECK(doc.grid.count(1) == 4);
    undo.break_merge();
    undo.push(std::make_unique<vs::CellsCommand>(doc, std::vector<vs::CellChange>{{0, 0, 2}}, "Paint", 8), true);
    CHECK(undo.size() == 2);
    undo.undo();
    CHECK(doc.grid.at(0, 0) == 1);
    undo.undo();
    CHECK(doc.grid.count(0) == 4);
}

TEST_CASE("overlapping changes in one stroke revert to the original") {
    vs::TileDocument doc{vs::TileGrid(2, 1, 5), {}};
    vs::UndoStack undo;
    undo.push(std::make_unique<vs::CellsCommand>(doc, std::vector<vs::CellChange>{{0, 0, 1}}, "P", 1), true);
    undo.push(std::make_unique<vs::CellsCommand>(doc, std::vector<vs::CellChange>{{0, 0, 2}}, "P", 1), true);
    CHECK(doc.grid.at(0, 0) == 2);
    undo.undo();
    CHECK(doc.grid.at(0, 0) == 5);
    undo.redo();
    CHECK(doc.grid.at(0, 0) == 2);
}

TEST_CASE("document command swaps the whole document") {
    vs::TileDocument doc{vs::TileGrid(2, 2, 0), {{"time", 0}}};
    vs::UndoStack undo;
    vs::TileDocument after = doc;
    after.grid.resize(5, 5, 1);
    after.properties["time"] = 300;
    undo.push(std::make_unique<vs::DocumentCommand>(doc, after, "Resize"));
    CHECK(doc.grid.width() == 5);
    CHECK(doc.properties["time"] == 300);
    CHECK_FALSE(undo.is_clean());
    undo.undo();
    CHECK(doc.grid.width() == 2);
    CHECK(doc.properties["time"] == 0);
    CHECK(undo.is_clean());
}
