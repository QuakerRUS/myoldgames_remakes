#include "soldier/recording.hpp"
#include "util.hpp"

using namespace soldier;
using test::make_level;

TEST_CASE("recording JSON round-trip") {
    Recording r;
    r.level = 5;
    r.player = "TEST";
    r.source = "unit";
    r.commands = {{0, Command::move(Dir::Left)}, {0, Command::turn_cw()}, {3, Command::use_item()},
                  {9, Command::next_item()}, {9, Command::give_up()}};
    r.result = RecordedResult{Status::Dead, 9, DeathCause::GaveUp};
    const Recording back = parse_recording_json(recording_to_json(r));
    CHECK(back.level == 5);
    CHECK(back.player == "TEST");
    CHECK(back.source == "unit");
    CHECK(back.commands == r.commands);
    CHECK(back.result == r.result);
}

TEST_CASE("recording JSON errors") {
    CHECK_THROWS_AS(parse_recording_json(R"({"format":"x","version":1})"), FormatError);
    CHECK_THROWS_AS(parse_recording_json(
                        R"({"format":"soldier-recording","version":1,"level":1,"player":"A","commands":[[1,"fly"]]})"),
                    FormatError);
    CHECK_THROWS_AS(parse_recording_json(
                        R"({"format":"soldier-recording","version":1,"level":1,"player":"A","commands":[[5,"up"],[4,"up"]]})"),
                    FormatError);
    const Recording ok = parse_recording_json(
        R"({"format":"soldier-recording","version":1,"level":1,"player":"A","original":{"seed":1},"commands":[]})");
    CHECK(ok.commands.empty());
    CHECK_FALSE(ok.result.has_value());
}

TEST_CASE("a live game, recorded and replayed, ends identically") {
    const Level lv = make_level({
        "#########",
        "#>.B._.D#",
        "#.r..T.X#",
        "#########",
    });
    Game g(lv);
    // Live input arriving at arbitrary moments, including during the cooldown.
    const std::vector<std::pair<int, Command>> input = {
        {0, Command::move(Dir::Right)}, {1, Command::move(Dir::Right)}, {2, Command::move(Dir::Down)},
        {2, Command::move(Dir::Left)},  {4, Command::turn_ccw()},      {5, Command::move(Dir::Up)},
        {7, Command::move(Dir::Right)}, {8, Command::move(Dir::Right)}, {13, Command::move(Dir::Right)},
        {15, Command::move(Dir::Right)}, {17, Command::move(Dir::Right)}, {19, Command::move(Dir::Right)},
        {21, Command::move(Dir::Right)}, {23, Command::move(Dir::Down)}, {25, Command::move(Dir::Right)},
    };
    std::size_t i = 0;
    while (g.status() == Status::Playing && g.tick_count() < 60) {
        while (i < input.size() && input[i].first == g.tick_count()) g.apply(input[i++].second);
        if (g.status() == Status::Playing) g.tick();
    }
    const Recording rec = parse_recording_json(recording_to_json(make_recording(g, 99, "ME")));
    const ReplayOutcome out = run_replay(lv, rec);
    CHECK(out.in_sync);
    CHECK(out.unused_commands == 0);
    CHECK(out.status == g.status());
    CHECK(out.tick == g.tick_count());
    Replayer r(lv, rec);
    while (!r.finished() && r.game().tick_count() < 60) r.step();
    CHECK(r.game().level().cells == g.level().cells);
}
