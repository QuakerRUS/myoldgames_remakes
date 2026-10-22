// Progress derived from records (save.json's level_reached cannot be raised by editing it).
#include <map>
#include <string>

#include "soldier/progress.hpp"
#include "util.hpp"

using namespace soldier;

namespace {

Level shipped_level(int n) { return load_level_file(test::data_path("levels/" + test::two_digits(n) + ".json")); }
Recording lisiy(int n) { return load_recording_file(test::data_path("records/LISIY/" + test::two_digits(n) + ".json")); }

int verify(std::map<int, Recording> records, std::string_view player) {
    return verified_level_reached(
        51, player, [](int n) -> std::optional<Level> { return shipped_level(n); },
        [&](int n) -> std::optional<Recording> {
            const auto it = records.find(n);
            return it == records.end() ? std::nullopt : std::optional<Recording>(it->second);
        });
}

}  // namespace

TEST_CASE("the shipped LISIY records prove every level") {
    CHECK(verified_level_reached(std::filesystem::path(SOLDIER_DATA_DIR) / "records" / "LISIY",
                                 std::filesystem::path(SOLDIER_DATA_DIR) / "levels", 51, "LISIY") == 52);
}

TEST_CASE("progress is 1 + the unbroken run of proven levels") {
    std::map<int, Recording> recs;
    CHECK(verify(recs, "LISIY") == 1);
    for (int n = 1; n <= 5; ++n) recs[n] = lisiy(n);
    CHECK(verify(recs, "LISIY") == 6);
    recs.erase(3);
    CHECK(verify(recs, "LISIY") == 3);  // levels 4 and 5 do not count without 3
}

TEST_CASE("records must belong to the player") {
    std::map<int, Recording> recs{{1, lisiy(1)}, {2, lisiy(2)}};
    CHECK(verify(recs, "BOB") == 1);
    recs[2].player = "BOB";
    recs[1].player = "BOB";
    CHECK(verify(recs, "BOB") == 3);  // the same moves, renamed: still a valid win for BOB
}

TEST_CASE("forged records do not count") {
    std::map<int, Recording> recs{{1, lisiy(1)}, {2, lisiy(2)}, {3, lisiy(3)}};
    SUBCASE("wrong end tick") { recs[2].result->tick -= 1; }
    SUBCASE("no moves") { recs[2].commands.clear(); }
    SUBCASE("not a win") { recs[2].result->status = Status::Dead; }
    SUBCASE("another level's record") { recs[2] = lisiy(1); }
    CHECK(verify(recs, "LISIY") == 2);
}

TEST_CASE("a fresh win made by playing proves the level") {
    const Level lv = shipped_level(1);
    Replayer r(lv, lisiy(1));
    while (!r.finished()) r.step();
    const Recording mine = make_recording(r.game(), 1, "NEWBIE");
    CHECK(record_proves_win(lv, mine, "NEWBIE"));
    CHECK_FALSE(record_proves_win(lv, mine, "OTHER"));
}
