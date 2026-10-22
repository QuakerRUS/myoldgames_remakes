#include <doctest/doctest.h>
#include <vs/engine/error.hpp>
#include <vs/engine/json_file.hpp>

#include <filesystem>

namespace fs = std::filesystem;

namespace {
fs::path temp_dir() {
    fs::path d = fs::temp_directory_path() / "vs_engine_tests" / "json";
    fs::remove_all(d);
    fs::create_directories(d);
    return d;
}
}  // namespace

TEST_CASE("json write and read round trip, with Cyrillic text and nested dirs") {
    const fs::path dir = temp_dir();
    const fs::path file = dir / "sub" / "save.json";
    nlohmann::json j;
    j["name"] = "ЛИСИЙ";
    j["level"] = 51;
    vs::write_json_file(file, j);
    const auto back = vs::read_json_file(file);
    REQUIRE(back.has_value());
    CHECK((*back)["name"] == "ЛИСИЙ");
    CHECK((*back)["level"] == 51);
    CHECK_FALSE(fs::exists(fs::path(file) += ".tmp"));
}

TEST_CASE("atomic write replaces an existing file") {
    const fs::path file = temp_dir() / "a.txt";
    vs::write_file_atomic(file, "first");
    vs::write_file_atomic(file, "second");
    CHECK(vs::read_text_file(file) == "second");
}

TEST_CASE("missing JSON is nullopt, malformed JSON throws") {
    const fs::path dir = temp_dir();
    CHECK_FALSE(vs::read_json_file(dir / "none.json").has_value());
    vs::write_file_atomic(dir / "bad.json", "{ not json");
    CHECK_THROWS_AS(vs::read_json_file(dir / "bad.json"), vs::Error);
    CHECK_THROWS_AS(vs::read_text_file(dir / "none.txt"), vs::Error);
}

TEST_CASE("UTF-8 path conversion") {
    const fs::path p = vs::utf8_to_path("данные/файл.json");
    CHECK(vs::path_to_utf8(p) == "данные/файл.json");
    const fs::path dir = temp_dir();
    vs::write_file_atomic(dir / vs::utf8_to_path("файл.txt"), "ok");
    CHECK(vs::read_text_file(dir / vs::utf8_to_path("файл.txt")) == "ok");
}

TEST_CASE("atomic write failure removes the temporary file and keeps the target") {
    const fs::path dir = temp_dir();
    // The target is a directory: the final rename fails.
    const fs::path target = dir / "taken";
    fs::create_directories(target / "inside");
    std::string what;
    try {
        vs::write_file_atomic(target, "data");
    } catch (const vs::Error& e) {
        what = e.what();
    }
    CHECK_FALSE(what.empty());
    CHECK(what.find("taken") != std::string::npos);
    CHECK(what.size() > std::string("cannot replace ").size() + vs::path_to_utf8(target).size());  // has a reason
    CHECK_FALSE(fs::exists(fs::path(target) += ".tmp"));
    CHECK(fs::is_directory(target / "inside"));

    // The parent "folder" is a file: nothing can be created, nothing is left behind.
    vs::write_file_atomic(dir / "plain", "x");
    CHECK_THROWS_AS(vs::write_file_atomic(dir / "plain" / "child.txt", "y"), vs::Error);
    CHECK(vs::read_text_file(dir / "plain") == "x");
}

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

TEST_CASE("atomic write onto a file locked without FILE_SHARE_DELETE fails cleanly") {
    const fs::path file = temp_dir() / "locked.txt";
    vs::write_file_atomic(file, "old");
    HANDLE h = CreateFileW(file.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
    REQUIRE(h != INVALID_HANDLE_VALUE);
    CHECK_THROWS_AS(vs::write_file_atomic(file, "new"), vs::Error);
    CloseHandle(h);
    CHECK_FALSE(fs::exists(fs::path(file) += ".tmp"));
    CHECK(vs::read_text_file(file) == "old");
    vs::write_file_atomic(file, "new");  // works once the lock is gone
    CHECK(vs::read_text_file(file) == "new");
}
#endif
