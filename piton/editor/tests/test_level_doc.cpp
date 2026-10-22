#include <doctest/doctest.h>

#include "level_doc.hpp"

using namespace piton_editor;
using piton::Cell;
using piton::Dir;

TEST_CASE("new document: empty field, start in the middle facing right") {
    const vs::TileDocument d = newDocument();
    CHECK(d.grid.width() == 38);
    CHECK(d.grid.height() == 23);
    CHECK(d.grid.count(kEmpty) == 38 * 23);
    CHECK(docStart(d) == Cell{19, 11});
    CHECK(docStartDir(d) == Dir::Right);
    CHECK(piton::checkLevel(toLevel(d)).ok());
}

TEST_CASE("start codes") {
    for (Dir d : piton::kAllDirs) CHECK(startCodeDir(startCode(d)) == d);
    CHECK_FALSE(startCodeDir(kBrick).has_value());
    CHECK_FALSE(startCodeDir(kEmpty).has_value());
}

TEST_CASE("symmetry painting") {
    const vs::TileDocument d = newDocument();
    CHECK(symmetricCells({0, 0}, Symmetry::None).size() == 1);
    CHECK(paintChanges(d, {0, 0}, kBrick, Symmetry::Horizontal) ==
          std::vector<vs::CellChange>{{0, 0, kBrick}, {37, 0, kBrick}});
    CHECK(paintChanges(d, {1, 2}, kBrick, Symmetry::Vertical) ==
          std::vector<vs::CellChange>{{1, 2, kBrick}, {1, 20, kBrick}});
    CHECK(paintChanges(d, {1, 2}, kBrick, Symmetry::Both).size() == 4);
    CHECK(paintChanges(d, {5, 11}, kBrick, Symmetry::Vertical).size() == 1);  // middle row
    CHECK(paintChanges(d, {1, 2}, kEmpty, Symmetry::Both).empty());           // nothing changes
}

TEST_CASE("start placement clears bricks under the snake") {
    vs::TileDocument d = cleared(newDocument());
    d.grid.fill(kBrick);
    const vs::TileDocument s = withStart(d, {10, 5}, Dir::Down);
    CHECK(docStart(s) == Cell{10, 5});
    CHECK(docStartDir(s) == Dir::Down);
    CHECK(s.grid.at(10, 5) == kEmpty);
    CHECK(s.grid.at(10, 4) == kEmpty);
    CHECK(s.grid.at(10, 3) == kEmpty);
    CHECK(s.grid.count(kEmpty) == 3);
}

TEST_CASE("tools") {
    vs::TileDocument d = newDocument();
    d.grid.set(0, 0, kBrick);
    d.grid.set(3, 4, kBrick);
    const vs::TileDocument b = borderFilled(d);
    CHECK(b.grid.count(kBrick) == 38 * 2 + 21 * 2 + 1);
    CHECK(cleared(b).grid.count(kBrick) == 0);

    const vs::TileDocument fx = flippedX(d);
    CHECK(fx.grid.at(34, 4) == kBrick);
    CHECK(docStart(fx) == Cell{18, 11});
    CHECK(docStartDir(fx) == Dir::Left);
    CHECK(flippedX(fx) == d);

    const vs::TileDocument fy = flippedY(d);
    CHECK(fy.grid.at(3, 18) == kBrick);
    CHECK(docStart(fy) == Cell{19, 11});
    CHECK(flippedY(fy) == d);

    const vs::TileDocument mx = mirroredLeftToRight(d);
    CHECK(mx.grid.at(3, 4) == kBrick);
    CHECK(mx.grid.at(34, 4) == kBrick);
    CHECK(mx.grid.at(37, 0) == kBrick);
    const vs::TileDocument my = mirroredTopToBottom(d);
    CHECK(my.grid.at(3, 18) == kBrick);
    CHECK(my.grid.at(0, 22) == kBrick);
}

TEST_CASE("all shipped levels round-trip byte-identically through the editor") {
    const RoundTripReport r = roundTrip(PITON_LEVELS_DIR);
    for (const std::string& l : r.lines) MESSAGE(l);
    CHECK(r.files == 10);
    CHECK(r.failures == 0);
}

TEST_CASE("document <-> level keeps everything") {
    piton::Level l;
    l.name = "Test \"quoted\" \xD0\x96";
    l.setTile({1, 1}, piton::Tile::Brick);
    l.setStart({5, 6}, Dir::Up);
    CHECK(toLevel(toDocument(l)) == l);
}

TEST_CASE("a start near the edge is moved inwards so the snake fits") {
    const vs::TileDocument d = newDocument();
    // (5,22) facing up: the body would be at (5,23), (5,24).
    const vs::TileDocument up = withStart(d, {5, 22}, Dir::Up);
    CHECK(docStart(up) == Cell{5, 20});
    CHECK(docStartDir(up) == Dir::Up);
    CHECK(withStart(d, {0, 3}, Dir::Right).properties == withStart(d, {2, 3}, Dir::Right).properties);
    CHECK(docStart(withStart(d, {37, 0}, Dir::Down)) == Cell{37, 2});
    CHECK(docStart(withStart(d, {36, 4}, Dir::Left)) == Cell{35, 4});
    for (Dir dir : piton::kAllDirs)
        for (int y = 0; y < 23; ++y)
            for (int x = 0; x < 38; ++x) {
                const vs::TileDocument s = withStart(d, {x, y}, dir);
                CHECK(piton::Level::startFits(docStart(s), dir));
                // The level the game would load: the snake inside, start cells free.
                const piton::Level lv = toLevel(s);
                CHECK(lv.startHead() == docStart(s));
                std::string err;
                piton::Level back;
                CHECK(piton::Level::fromJson(lv.toJson(), back, &err));
            }
}

TEST_CASE("bricks are never painted on the start snake") {
    const vs::TileDocument d = newDocument();  // start (19,11) facing right: (19..17, 11)
    CHECK(paintChanges(d, {18, 11}, kBrick, Symmetry::None).empty());
    CHECK(paintChanges(d, {18, 10}, kBrick, Symmetry::None).size() == 1);
    // A mirror image landing on the start is skipped, the painted cell is not.
    const vs::TileDocument s = withStart(d, {20, 5}, Dir::Right);  // (20..18, 5)
    CHECK(paintChanges(s, {17, 5}, kBrick, Symmetry::Horizontal) == std::vector<vs::CellChange>{{17, 5, kBrick}});
    // Whole-document tools keep the start free.
    const vs::TileDocument edge = withStart(d, {2, 0}, Dir::Right);
    const vs::TileDocument b = borderFilled(edge);
    for (Cell c : toLevel(edge).startCells()) CHECK(b.grid.at(c.x, c.y) == kEmpty);
    CHECK(piton::checkLevel(toLevel(b)).playable());
}

TEST_CASE("a brick under the start (hand-edited document) is reported, not hidden") {
    vs::TileDocument d = newDocument();
    d.grid.set(18, 11, kBrick);
    const piton::LevelCheck c = piton::checkLevel(toLevel(d));
    CHECK_FALSE(c.playable());
}
