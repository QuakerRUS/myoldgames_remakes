#include "test_util.hpp"

#include <piton/demo_pilot.hpp>

#include <algorithm>
#include <cstdio>

using namespace piton;

namespace {

struct RunResult {
    GameState state;
    int ticks;
    int maxLength;
    int eaten;
};

RunResult runDemo(const Level& lv, std::uint64_t seed, int budget) {
    Game g(lv, seed);
    DemoPilot pilot;
    int maxLen = g.length();
    while (g.state() == GameState::Running && g.ticks() < budget) {
        const Dir d = pilot.choose(g);
        CHECK(d != opposite(g.heading()));
        g.steer(d);
        g.tick();
        maxLen = std::max(maxLen, g.length());
    }
    return {g.state(), g.ticks(), maxLen, g.rabbitsEaten()};
}

} // namespace

TEST_CASE("DemoPilot completes every level for several seeds") {
    constexpr int kBudget = 10000;
    constexpr std::uint64_t kSeeds[] = {1, 2, 3, 1234, 987654321};
    std::printf("level  name            min/avg/max ticks   max length\n");
    for (int n = 1; n <= kLevelCount; ++n) {
        const Level lv = loadTestLevel(n);
        int minT = 1 << 30, maxT = 0, sumT = 0, maxLen = 0, runs = 0;
        for (std::uint64_t seed : kSeeds) {
            CAPTURE(n);
            CAPTURE(seed);
            RunResult r = runDemo(lv, seed, kBudget);
            CAPTURE(r.eaten);
            CAPTURE(r.ticks);
            CHECK(r.state == GameState::LevelComplete);
            CHECK(r.eaten == kRabbitsPerLevel);
            minT = std::min(minT, r.ticks);
            maxT = std::max(maxT, r.ticks);
            sumT += r.ticks;
            maxLen = std::max(maxLen, r.maxLength);
            ++runs;
        }
        std::printf("  %02d   %-14s  %5d/%5d/%5d   %d\n", n, lv.name.c_str(), minT, sumT / runs, maxT, maxLen);
    }
}

TEST_CASE("DemoPilot survives with a long target on an open field") {
    // 150 rabbits on the open level: the snake grows to 153 cells.
    Game g(loadTestLevel(1), 99, 150);
    DemoPilot pilot;
    while (g.state() == GameState::Running && g.ticks() < 100000) {
        pilot.drive(g);
        g.tick();
    }
    CAPTURE(g.rabbitsEaten());
    CHECK(g.state() == GameState::LevelComplete);
}
