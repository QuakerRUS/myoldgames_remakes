// Every LISIY demo of the original, replayed on its 1.1 level, must reach the exit at the same
// tick as the original replay loop (the expected "result" was computed by
// tools/import/soldier_records.py with an exact transliteration of GAME1.BAS).
#include <string>

#include "soldier/recording.hpp"
#include "util.hpp"

using namespace soldier;

TEST_CASE("all LISIY records finish with Won") {
    int checked = 0;
    for (int n = 1; n <= 51; ++n) {
        CAPTURE(n);
        const std::string nn = test::two_digits(n);
        const Level lv = load_level_file(test::data_path("levels/" + nn + ".json"));
        const Recording rec = load_recording_file(test::data_path("records/LISIY/" + nn + ".json"));
        CHECK(rec.level == n);
        CHECK(rec.player == "LISIY");
        REQUIRE(rec.result.has_value());
        CHECK(rec.result->status == Status::Won);

        const ReplayOutcome out = run_replay(lv, rec);
        CHECK(out.status == Status::Won);
        CHECK(out.tick == rec.result->tick);
        CHECK(out.unused_commands == 0);
        CHECK(out.in_sync);
        ++checked;
    }
    CHECK(checked == 51);
}

TEST_CASE("records on timed levels finish before the timer runs out") {
    for (int n : {1, 24, 33, 34}) {
        CAPTURE(n);
        const std::string nn = test::two_digits(n);
        const Level lv = load_level_file(test::data_path("levels/" + nn + ".json"));
        const Recording rec = load_recording_file(test::data_path("records/LISIY/" + nn + ".json"));
        Replayer r(lv, rec);
        while (!r.finished()) r.step();
        CHECK(r.game().status() == Status::Won);
        CHECK(r.game().timer() >= 0);
        CHECK(r.game().tick_count() < 4 * (lv.time_limit + 1));
    }
}

TEST_CASE("players.json") {
    const std::string text = read_text_file(test::data_path("players.json"));
    CHECK(text.find("\"LISIY\"") != std::string::npos);
    CHECK(text.find("\"level_reached\": 52") != std::string::npos);
}
