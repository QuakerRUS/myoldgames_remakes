#include <doctest/doctest.h>

#include <filesystem>
#include <memory>

#include <vs/engine/json_file.hpp>
#include <vs/engine/undo_stack.hpp>

#include "commands.hpp"
#include "document.hpp"
#include "path_util.hpp"

namespace fs = std::filesystem;

namespace {
fs::path temp_dir() {
    fs::path d = fs::temp_directory_path() / "painter_tests" / "document";
    fs::remove_all(d);
    fs::create_directories(d);
    return d;
}

painter::Document new_sheet() {
    painter::Document doc;
    doc.image = vs::Image(4, 4, {255, 0, 0, 255});
    doc.sheet = painter::Sheet{};
    doc.sheet->data.image = "untitled.png";
    return doc;
}
}  // namespace

TEST_CASE("lower_ext is UTF-8 safe and lowers ASCII only") {
    CHECK(painter::lower_ext("Hero.PNG") == ".png");
    CHECK(painter::lower_ext(vs::utf8_to_path("герой.JsOn")) == ".json");
    CHECK(painter::lower_ext(vs::utf8_to_path("файл.ПНГ")) == ".ПНГ");
    CHECK(painter::lower_ext("noext").empty());
}

TEST_CASE("save targets and foreign companions of a sheet") {
    const fs::path dir = temp_dir();
    painter::Document doc = new_sheet();

    const painter::SaveTargets a = painter::save_targets(doc, dir / "hero.png");
    CHECK(a.png == dir / "hero.png");
    CHECK(a.json == dir / "hero.json");
    CHECK(a.companion == dir / "hero.json");
    CHECK(painter::foreign_companion(doc, dir / "hero.png").empty());  // nothing on disk yet

    // hero.json belongs to another document: saving hero.png would overwrite it.
    vs::write_file_atomic(dir / "hero.json", "{}");
    CHECK(painter::foreign_companion(doc, dir / "hero.png") == dir / "hero.json");
    // level.json -> level.png
    vs::write_file_atomic(dir / "level.png", "x");
    CHECK(painter::foreign_companion(doc, dir / "level.json") == dir / "level.png");

    // After saving as own.json, own.png is this document's companion: no question on Save.
    painter::save_document(doc, dir / "own.json");
    CHECK(doc.path == dir / "own.json");
    CHECK(fs::exists(dir / "own.png"));
    CHECK(painter::foreign_companion(doc, dir / "own.json").empty());

    // A plain image has no companion.
    painter::Document img;
    img.image = vs::Image(2, 2);
    CHECK(painter::foreign_companion(img, dir / "hero.png").empty());
}

TEST_CASE("undo commands report their pixel memory for the byte budget") {
    painter::Document doc;
    doc.image = vs::Image(100, 100);
    vs::UndoStack undo(0, 100 * 100 * 4 * 3);  // room for about 1.5 full-image steps
    for (int i = 0; i < 4; ++i)
        undo.push(std::make_unique<painter::ImageCommand>(doc, doc.image, vs::Image(100, 100), std::nullopt, std::nullopt, "img"));
    CHECK(undo.bytes() >= std::size_t{100 * 100 * 4 * 2});
    CHECK(undo.bytes() <= undo.byte_limit());
    CHECK(undo.size() == 1);
    painter::RegionCommand small(doc, {0, 0, 10, 10}, vs::Image(10, 10), vs::Image(10, 10), "r");
    CHECK(small.memory_bytes() >= std::size_t{10 * 10 * 4 * 2});
}
