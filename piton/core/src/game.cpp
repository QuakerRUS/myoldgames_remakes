#include "piton/game.hpp"

namespace piton {

std::uint64_t Rng::next() {
    std::uint64_t z = (state_ += 0x9E3779B97F4A7C15ull);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    return z ^ (z >> 31);
}

std::uint32_t Rng::below(std::uint32_t n) {
    // Lemire's multiply-shift on the high 32 bits; bias is negligible for n <= 874.
    return static_cast<std::uint32_t>(((next() >> 32) * n) >> 32);
}

Game::Game(const Level& level, std::uint64_t seed, int rabbitsToWin)
    : level_(level), rng_(seed), target_(rabbitsToWin), heading_(level.startDir()) {
    for (Cell c : level_.startCells()) {
        body_.push_back(c);
        setOccupied(c, true);
    }
    prevHead_ = head();
    prevTail_ = tail();
    if (!spawnRabbit()) completeLevel(head());
}

bool Game::isOnSnake(Cell c) const {
    return insideField(c) && occ_[static_cast<size_t>(cellIndex(c))];
}

bool Game::queueTurn(Dir d) {
    if (state_ != GameState::Running) return false;
    if (static_cast<int>(queue_.size()) >= kMaxQueuedTurns) return false;
    Dir base = queue_.empty() ? heading_ : queue_.back();
    if (d == base || d == opposite(base)) return false;
    queue_.push_back(d);
    return true;
}

bool Game::steer(Dir d) {
    queue_.clear();
    if (d == heading_) return true;
    return queueTurn(d);
}

bool Game::placeRabbit(Cell c) {
    if (state_ != GameState::Running || !isFree(c)) return false;
    rabbit_ = c;
    return true;
}

bool Game::spawnRabbit() {
    rabbit_.reset();
    std::vector<Cell> free;
    free.reserve(kCellCount);
    for (int i = 0; i < kCellCount; ++i) {
        Cell c = cellAt(i);
        if (isFree(c)) free.push_back(c);
    }
    if (free.empty()) return false;
    Cell c = free[rng_.below(static_cast<std::uint32_t>(free.size()))];
    rabbit_ = c;
    Event e;
    e.type = EventType::RabbitSpawned;
    e.cell = c;
    events_.push_back(e);
    return true;
}

void Game::completeLevel(Cell at) {
    state_ = GameState::LevelComplete;
    queue_.clear();
    Event lc;
    lc.type = EventType::LevelComplete;
    lc.value = eaten_;
    lc.cell = at;
    events_.push_back(lc);
}

const std::vector<Event>& Game::tick() {
    events_.clear();
    if (state_ != GameState::Running) return events_;

    Dir prev = heading_;
    if (!queue_.empty()) {
        heading_ = queue_.front();
        queue_.pop_front();
    }
    if (heading_ != prev) {
        Event e;
        e.type = EventType::Turned;
        e.cell = head();
        e.dir = heading_;
        e.prevDir = prev;
        events_.push_back(e);
    }

    Cell next = step(head(), heading_);
    bool eats = rabbit_ && *rabbit_ == next;
    DeathCause cause = DeathCause::None;
    if (!insideField(next)) cause = DeathCause::Border;
    else if (level_.isBrick(next)) cause = DeathCause::Brick;
    else if (isOnSnake(next) && !(next == tail() && !eats)) cause = DeathCause::Self;
    if (cause != DeathCause::None) {
        state_ = GameState::GameOver;
        death_ = cause;
        queue_.clear();
        prevHead_ = head();
        prevTail_ = tail();
        grew_ = false;
        Event e;
        e.type = EventType::Died;
        e.cell = next;
        e.dir = heading_;
        e.cause = cause;
        events_.push_back(e);
        return events_;
    }

    prevHead_ = head();
    prevTail_ = tail();
    grew_ = eats;
    if (!eats) {
        setOccupied(tail(), false);
        body_.pop_back();
    }
    body_.push_front(next);
    setOccupied(next, true);
    ++ticks_;

    Event mv;
    mv.type = EventType::Moved;
    mv.cell = next;
    mv.dir = heading_;
    events_.push_back(mv);

    if (eats) {
        ++eaten_;
        rabbit_.reset();
        Event e;
        e.type = EventType::Ate;
        e.cell = next;
        e.value = eaten_;
        events_.push_back(e);
        if (eaten_ >= target_ || !spawnRabbit()) completeLevel(next);
    }
    return events_;
}

std::vector<Segment> Game::segments() const {
    std::vector<Segment> out(body_.size());
    const size_t n = body_.size();
    for (size_t i = 0; i < n; ++i) {
        Segment& s = out[i];
        s.cell = body_[i];
        // out: towards the head-side neighbour (i-1); in: from the tail-side neighbour (i+1).
        Dir o = heading_;
        if (i > 0) o = dirBetween(body_[i], body_[i - 1]).value_or(heading_);
        Dir in = o;
        if (i + 1 < n) in = dirBetween(body_[i + 1], body_[i]).value_or(o);
        if (i == 0) in = o = heading_;
        if (i + 1 == n) in = o;
        s.in = in;
        s.out = o;
    }
    return out;
}

} // namespace piton
