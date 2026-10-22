#include "piton/demo_pilot.hpp"

#include <array>
#include <deque>
#include <vector>

namespace piton {

namespace {

using Body = std::deque<Cell>;
constexpr int kUnreached = -1;

struct Bfs {
    std::array<int, kCellCount> dist;
    std::array<int, kCellCount> prev;
};

// Time-aware BFS from body.front(). The cell of body segment i (0 = head) becomes free
// after (length - i) moves, so it may be entered at distance >= that. `blocked` (the
// rabbit, when it must not be eaten on the way) is never entered.
void runBfs(const Level& level, const Body& body, Bfs& out, std::optional<Cell> blocked = std::nullopt) {
    std::array<int, kCellCount> freeAt;
    freeAt.fill(0);
    const int len = static_cast<int>(body.size());
    for (int i = 0; i < len; ++i) {
        const Cell c = body[static_cast<size_t>(i)];
        if (insideField(c)) freeAt[static_cast<size_t>(cellIndex(c))] = len - i;
    }
    if (blocked && insideField(*blocked)) freeAt[static_cast<size_t>(cellIndex(*blocked))] = kCellCount * 4;
    out.dist.fill(kUnreached);
    out.prev.fill(-1);
    const int start = cellIndex(body.front());
    out.dist[static_cast<size_t>(start)] = 0;
    std::array<int, kCellCount> q;
    size_t qn = 0;
    q[qn++] = start;
    for (size_t qi = 0; qi < qn; ++qi) {
        const int ci = q[qi];
        const Cell c = cellAt(ci);
        const int nd = out.dist[static_cast<size_t>(ci)] + 1;
        for (Dir d : kAllDirs) {
            const Cell n = step(c, d);
            if (!level.isFree(n)) continue;
            const int ni = cellIndex(n);
            if (out.dist[static_cast<size_t>(ni)] != kUnreached) continue;
            if (freeAt[static_cast<size_t>(ni)] > nd) continue;
            out.dist[static_cast<size_t>(ni)] = nd;
            out.prev[static_cast<size_t>(ni)] = ci;
            q[qn++] = ni;
        }
    }
}

// Cells from the head (exclusive) to target (inclusive); empty if unreachable. Among the
// shortest paths it prefers straight runs (fewer turns leave more room in corridors).
std::vector<Cell> pathTo(const Bfs& bfs, Cell target) {
    std::vector<Cell> path;
    int i = cellIndex(target);
    if (bfs.dist[static_cast<size_t>(i)] <= 0) return path;
    std::optional<Dir> last;  // direction of the step leaving the current cell
    while (bfs.dist[static_cast<size_t>(i)] > 0) {
        path.push_back(cellAt(i));
        const Cell c = cellAt(i);
        const int want = bfs.dist[static_cast<size_t>(i)] - 1;
        int pick = -1;
        std::optional<Dir> pickDir;
        for (Dir d : kAllDirs) {
            const Cell p = step(c, opposite(d));  // predecessor p, step p -> c goes `d`
            if (!insideField(p) || bfs.dist[static_cast<size_t>(cellIndex(p))] != want) continue;
            if (pick < 0 || (last && d == *last && !(pickDir && *pickDir == *last))) {
                pick = cellIndex(p);
                pickDir = d;
            }
        }
        if (pick < 0) return {};
        last = pickDir;
        i = pick;
    }
    return {path.rbegin(), path.rend()};
}

// Moves the virtual snake one cell; grows when it enters the rabbit.
void moveBody(Body& body, Cell next, std::optional<Cell> rabbit) {
    if (!(rabbit && *rabbit == next)) body.pop_back();
    body.push_front(next);
}

struct Safety {
    bool tailReachable = false;
    int tailDist = kUnreached;
    int area = 0;  // cells reachable from the head (time-aware)
};

// How safe is the virtual snake `v`? `avoid` is a rabbit still on the field that the
// check must not run through: eating it would delay the tail by one move.
Safety assess(const Level& level, const Body& v, std::optional<Cell> avoid) {
    Bfs bfs;
    runBfs(level, v, bfs, avoid);
    Safety s;
    s.tailDist = v.size() < 2 ? 0 : bfs.dist[static_cast<size_t>(cellIndex(v.back()))];
    s.tailReachable = s.tailDist != kUnreached;
    for (int d : bfs.dist)
        if (d > 0) ++s.area;
    return s;
}

bool legalMove(const Game& game, Dir d) {
    if (d == opposite(game.heading())) return false;
    const Cell n = step(game.head(), d);
    if (!game.level().isFree(n)) return false;
    const bool eats = game.rabbit() && *game.rabbit() == n;
    if (game.isOnSnake(n) && !(n == game.tail() && !eats)) return false;
    return true;
}

} // namespace

Dir DemoPilot::choose(const Game& game) {
    if (game.rabbitsEaten() != lastEaten_) {
        lastEaten_ = game.rabbitsEaten();
        stepsWithoutFood_ = 0;
    } else {
        ++stepsWithoutFood_;
    }
    const Level& level = game.level();
    const Body& body = game.body();
    const Cell head = game.head();
    const auto rabbit = game.rabbit();
    const int freeCells = level.freeCellCount();
    const bool restless = stepsWithoutFood_ > freeCells;
    const bool starving = stepsWithoutFood_ > 4 * freeCells;

    // 1. The shortest path to the rabbit whose end state keeps the tail reachable; one
    //    candidate path per first move.
    if (rabbit) {
        std::optional<Dir> best;
        int bestLen = 0;
        for (Dir d : kAllDirs) {
            if (!legalMove(game, d)) continue;
            const Cell n = step(head, d);
            Body v = body;
            moveBody(v, n, rabbit);
            int len = 1;
            if (!(n == *rabbit)) {
                Bfs bfs;
                runBfs(level, v, bfs);
                const std::vector<Cell> path = pathTo(bfs, *rabbit);
                if (path.empty()) continue;
                for (Cell c : path) moveBody(v, c, rabbit);
                len += static_cast<int>(path.size());
            }
            const Safety s = assess(level, v, std::nullopt);
            const bool safe = s.tailReachable || (starving && s.area >= static_cast<int>(v.size()));
            if (!safe) continue;
            if (!best || len < bestLen || (len == bestLen && d == game.heading())) {
                best = d;
                bestLen = len;
            }
        }
        if (best) return *best;
    }

    // 2. A move that keeps the tail reachable: the one farthest from the tail. After a long
    //    time without food, sometimes a random one, to break endless loops.
    std::optional<Dir> best;
    int bestScore = -1;
    Dir safeMoves[4];
    int safeCount = 0;
    for (Dir d : kAllDirs) {
        if (!legalMove(game, d)) continue;
        const Cell n = step(head, d);
        Body v = body;
        moveBody(v, n, rabbit);
        const bool ate = rabbit && *rabbit == n;
        const Safety s = assess(level, v, ate ? std::nullopt : rabbit);
        if (!s.tailReachable) continue;
        safeMoves[safeCount++] = d;
        if (s.tailDist > bestScore) {
            bestScore = s.tailDist;
            best = d;
        }
    }
    if (restless && safeCount > 1 && rng_.below(4) == 0) return safeMoves[rng_.below(static_cast<std::uint32_t>(safeCount))];
    if (best) return *best;

    // 3. The largest reachable area.
    for (Dir d : kAllDirs) {
        if (!legalMove(game, d)) continue;
        Body v = body;
        moveBody(v, step(head, d), rabbit);
        const Safety s = assess(level, v, std::nullopt);
        if (s.area > bestScore) {
            bestScore = s.area;
            best = d;
        }
    }
    if (best) return *best;
    return game.heading();  // no legal move: the snake dies whatever it does
}

} // namespace piton
