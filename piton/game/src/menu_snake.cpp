#include "menu_snake.hpp"

#include <cmath>

#include "assets.hpp"
#include "field_renderer.hpp"
#include "snake_renderer.hpp"
#include "ui_draw.hpp"

namespace piton_game {
namespace {

constexpr int kLength = 9;           // snake segments
constexpr double kStep = 0.15;       // seconds per cell
constexpr int kHopCells = 2;         // the rabbit hops two cells at a time ...
constexpr double kHopTime = 0.30;    // ... a little faster than the snake crawls
constexpr int kScareDistance = 4;    // the rabbit hops when the head is this close

}  // namespace

void MenuSnake::init(const Assets& a, vs::Vec2 origin, int cols, int rows) {
    origin_ = origin;
    cols_ = cols;
    rows_ = rows;
    ring_.clear();
    for (int x = 0; x < cols; ++x) ring_.push_back({x, 0});
    for (int y = 1; y < rows; ++y) ring_.push_back({cols - 1, y});
    for (int x = cols - 2; x >= 0; --x) ring_.push_back({x, rows - 1});
    for (int y = rows - 2; y >= 1; --y) ring_.push_back({0, y});

    grass_.clear();
    for (int y = 0; y < rows; ++y)
        for (int x = 0; x < cols; ++x) {
            const unsigned h = static_cast<unsigned>(x * 7919 + y * 104729 + 17) * 2654435761u >> 16;
            grass_.push_back(h % 23 == 0 ? "grass_6" : h % 29 == 0 ? "grass_7" : "grass_" + std::to_string(h % 6));
        }

    head_ = kLength + 2;
    step_time_ = 0;
    head_anim_.init(a.snake);
    rabbit_ = head_ + 9;
    hop_time_ = -1;
    rabbit_anim_.play(a.rabbit.animation("rabbit_idle"), true);
}

piton::Cell MenuSnake::track(int index) const {
    const int n = ring_size();
    return ring_[static_cast<std::size_t>(((index % n) + n) % n)];
}

void MenuSnake::update(double dt) {
    if (ring_.empty()) return;
    head_anim_.update(dt);
    rabbit_anim_.update(dt);
    step_time_ += dt;
    while (step_time_ >= kStep) {
        step_time_ -= kStep;
        head_ = (head_ + 1) % ring_size();
    }
    if (hop_time_ >= 0) {
        hop_time_ += dt;
        if (hop_time_ >= kHopTime) {
            hop_time_ = -1;
            rabbit_ = (rabbit_ + kHopCells) % ring_size();
        }
    }
    const int dist = ((rabbit_ - head_) % ring_size() + ring_size()) % ring_size();
    if (hop_time_ < 0 && dist <= kScareDistance) {
        hop_time_ = 0;
        const piton::Cell from = track(rabbit_);
        const piton::Cell to = track(rabbit_ + kHopCells);
        if (to.x > from.x) rabbit_flip_ = true;
        if (to.x < from.x) rabbit_flip_ = false;
    }
    head_anim_.set_mouth_open(dist <= 2);
}

void MenuSnake::draw_lawn(vs::Renderer& r, const Assets& a) const {
    draw_wood_frame(r, a, {origin_.x, origin_.y, kCell * static_cast<float>(cols_), kCell * static_cast<float>(rows_)},
                    16.0f);
    for (int y = 0; y < rows_; ++y)
        for (int x = 0; x < cols_; ++x)
            a.tiles.draw(r, grass_[static_cast<std::size_t>(y * cols_ + x)],
                         {origin_.x + static_cast<float>(x) * kCell, origin_.y + static_cast<float>(y) * kCell});
}

void MenuSnake::draw_actors(vs::Renderer& r, const Assets& a) const {
    if (ring_.empty()) return;
    // Rabbit (hops along the track with a little arc).
    const piton::Cell from = track(rabbit_);
    vs::Vec2 p{static_cast<float>(from.x), static_cast<float>(from.y)};
    float lift = 0;
    std::string frame = rabbit_anim_.frame();
    if (hop_time_ >= 0) {
        const float k = static_cast<float>(hop_time_ / kHopTime);
        const piton::Cell to = track(rabbit_ + kHopCells);
        const piton::Cell mid = track(rabbit_ + 1);
        // Follow the track cell by cell so the rabbit turns the corners properly.
        const float seg = k * kHopCells;
        const piton::Cell c0 = seg < 1 ? from : mid;
        const piton::Cell c1 = seg < 1 ? mid : to;
        const float f = seg < 1 ? seg : seg - 1;
        p = {static_cast<float>(c0.x) + (static_cast<float>(c1.x - c0.x)) * f,
             static_cast<float>(c0.y) + (static_cast<float>(c1.y - c0.y)) * f};
        lift = std::sin(k * 3.14159265f) * 7.0f;
        frame = k < 0.2f || k > 0.8f ? "rabbit_hop1" : "rabbit_hop2";
    }
    a.rabbit.draw(r, frame,
                  {std::round(origin_.x + p.x * kCell), std::round(origin_.y + p.y * kCell - lift)},
                  {.flip = rabbit_flip_ ? vs::Flip::Horizontal : vs::Flip::None});

    // Snake.
    SnakePose pose;
    for (int i = 0; i < kLength; ++i) pose.body.push_back(track(head_ - i));
    pose.heading = piton::dirBetween(track(head_ - 1), track(head_)).value_or(piton::Dir::Right);
    pose.prev_head = track(head_ - 1);
    pose.prev_tail = track(head_ - kLength);
    pose.progress = static_cast<float>(step_time_ / kStep);
    pose.head_frame = head_anim_.frame();
    draw_snake(r, a.snake, origin_, pose);
}

}  // namespace piton_game
