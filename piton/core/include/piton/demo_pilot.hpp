#pragma once
// The computer player for the DEMO mode.

#include "piton/game.hpp"

namespace piton {

class DemoPilot {
public:
    // The seed only drives the occasional random move that breaks endless loops.
    explicit DemoPilot(std::uint64_t seed = 0x5EED) : rng_(seed) {}

    // Direction for the next tick (never opposite to game.heading()). Strategy, in order:
    //  1. for each legal first move, the shortest path (time-aware BFS: body cells count as
    //     free once the tail has left them; straight runs preferred) to the rabbit; the
    //     shortest one is taken whose end state still lets the head reach the tail;
    //  2. otherwise a move after which the tail stays reachable, the one farthest from the
    //     tail (buys time until the field opens up); after a long time without food, now
    //     and then a random such move to break endless loops;
    //  3. otherwise the move with the largest reachable area (flood fill);
    //  4. after a very long time without food, step 1 also accepts paths whose end state
    //     merely leaves at least as many reachable cells as the snake is long.
    // Call it every tick, in order, for one game (it counts ticks without food).
    Dir choose(const Game& game);

    // game.steer(choose(game)). Call once before every game.tick().
    void drive(Game& game) { game.steer(choose(game)); }

private:
    Rng rng_;
    int lastEaten_ = -1;
    int stepsWithoutFood_ = 0;
};

} // namespace piton
