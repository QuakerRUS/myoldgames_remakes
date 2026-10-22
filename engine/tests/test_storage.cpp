#include <doctest/doctest.h>
#include <vs/engine/app.hpp>
#include <vs/engine/storage.hpp>

#include <filesystem>

namespace fs = std::filesystem;

namespace {
// Restores the global user_data_dir state after a test.
struct StorageStateGuard {
    fs::path override_dir = vs::user_data_dir_override();
    bool smoke = vs::user_data_smoke_mode();
    ~StorageStateGuard() {
        vs::set_user_data_dir_override(override_dir);
        vs::set_user_data_smoke_mode(smoke);
    }
};
}  // namespace

TEST_CASE("user_data_dir override is returned and created") {
    StorageStateGuard guard;
    const fs::path dir = fs::temp_directory_path() / "vs_engine_tests" / "data_dir" / "nested";
    fs::remove_all(dir.parent_path());
    vs::set_user_data_dir_override(dir);
    CHECK(vs::user_data_dir("Soldier") == dir);
    CHECK(vs::user_data_dir("Piton") == dir);
    CHECK(fs::is_directory(dir));
}

TEST_CASE("user_data_dir in smoke mode uses the temporary folder, not %APPDATA%") {
    StorageStateGuard guard;
    vs::set_user_data_dir_override({});
    vs::set_user_data_smoke_mode(true);
    const fs::path dir = vs::user_data_dir("SmokeGame");
    CHECK(dir == fs::temp_directory_path() / "vs-smoke" / "SmokeGame");
    CHECK(fs::is_directory(dir));
    // An override wins over smoke mode.
    const fs::path over = fs::temp_directory_path() / "vs_engine_tests" / "data_dir_over";
    vs::set_user_data_dir_override(over);
    CHECK(vs::user_data_dir("SmokeGame") == over);
}

TEST_CASE("parse_common_args: --data-dir and --smoke") {
    StorageStateGuard guard;
    vs::set_user_data_dir_override({});
    vs::set_user_data_smoke_mode(false);
    const fs::path dir = fs::temp_directory_path() / "vs_engine_tests" / "args_dir";
    const std::string dir_s = vs::path_to_utf8(dir);
    std::string a0 = "game", a1 = "--smoke", a2 = "--data-dir", a3 = dir_s, a4 = "level.json";
    char* argv[] = {a0.data(), a1.data(), a2.data(), a3.data(), a4.data()};
    vs::AppConfig cfg;
    const auto rest = vs::parse_common_args(5, argv, cfg);
    REQUIRE(rest.size() == 1);
    CHECK(rest[0] == "level.json");
    CHECK(cfg.smoke);
    CHECK(cfg.quit_after > 0);
    CHECK(cfg.data_dir == dir);
    CHECK(vs::user_data_dir_override() == dir);
    CHECK(vs::user_data_dir("X") == dir);

    std::string b1 = "--data-dir=" + dir_s + "2";
    char* argv2[] = {a0.data(), b1.data()};
    vs::AppConfig cfg2;
    CHECK(vs::parse_common_args(2, argv2, cfg2).empty());
    CHECK(cfg2.data_dir == vs::utf8_to_path(dir_s + "2"));
    CHECK_FALSE(cfg2.smoke);
}
