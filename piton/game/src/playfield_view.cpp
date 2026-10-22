#include "playfield_view.hpp"

#include <cmath>

#include "assets.hpp"
#include "palette.hpp"
#include "snake_renderer.hpp"
#include "ui_draw.hpp"

namespace piton_game {

void PlayfieldView::init(const Assets& a, const piton::Game& game, unsigned seed) {
    field_.build(game.level(), seed);
    rabbit_.init(a.rabbit);
    head_.init(a.snake);
    popups_.clear();
    if (game.rabbit()) rabbit_.spawn(*game.rabbit());
}

void PlayfieldView::on_events(const piton::Game& game, const std::vector<piton::Event>& events, int points,
                              double tick_seconds) {
    for (const piton::Event& e : events) {
        switch (e.type) {
        case piton::EventType::Ate:
            // The head needs about half a tick to cover the rabbit.
            rabbit_.eaten(tick_seconds * 0.5);
            head_.eat();
            popups_.push_back({{kFieldOrigin.x + static_cast<float>(e.cell.x) * kCell + 12,
                                kFieldOrigin.y + static_cast<float>(e.cell.y) * kCell - 4},
                               "+" + std::to_string(points)});
            break;
        case piton::EventType::RabbitSpawned: rabbit_.spawn(e.cell); break;
        case piton::EventType::Died: head_.die(); break;
        case piton::EventType::LevelComplete: break;
        case piton::EventType::Turned:
        case piton::EventType::Moved: break;
        }
    }
    // Open the mouth when the next move enters the rabbit's cell.
    const std::optional<piton::Cell> rabbit = game.rabbit();
    head_.set_mouth_open(game.state() == piton::GameState::Running && rabbit &&
                         piton::step(game.head(), game.nextHeading()) == *rabbit);
}

void PlayfieldView::update(double dt, const piton::Game& game) {
    (void)game;
    clock_ += dt;
    rabbit_.update(dt);
    head_.update(dt);
    if (shake_ > 0) shake_ -= dt;
    for (Popup& p : popups_) p.age += dt;
    std::erase_if(popups_, [](const Popup& p) { return p.age > 0.9; });
}

void PlayfieldView::draw(vs::Renderer& r, const Assets& a, const piton::Game& game, float progress) const {
    vs::Vec2 o = kFieldOrigin;
    if (shake_ > 0) {
        const float k = static_cast<float>(shake_ / 0.35);
        o.x += std::round(static_cast<float>(std::sin(clock_ * 90.0)) * 5.0f * k);
        o.y += std::round(static_cast<float>(std::cos(clock_ * 70.0)) * 3.0f * k);
    }
    field_.draw(r, a.tiles, o);
    rabbit_.draw(r, o);

    const std::deque<piton::Cell>& body = game.body();
    SnakePose pose = pose_from_body(std::vector<piton::Cell>(body.begin(), body.end()), game.heading(),
                                    game.prevHead(), game.prevTail(), progress, head_.frame());
    draw_snake(r, a.snake, o, pose);

    for (const Popup& p : popups_) {
        const float k = static_cast<float>(p.age / 0.9);
        const auto alpha = static_cast<std::uint8_t>(255 * std::min(1.0f, 2.0f * (1.0f - k)));
        draw_text(r, a.font16, p.text, {p.pos.x, p.pos.y - 26 * k}, pal::kGold.with_alpha(alpha), vs::Align::Center,
                  TextFx::Both);
    }
}

}  // namespace piton_game
