#include "event_sounds.hpp"

#include <algorithm>
#include <cmath>

#include "level_view.hpp"
#include "resources.hpp"

namespace sg {

using soldier::EventType;

namespace {

// Stereo position of a cell relative to the player.
float pan_of(soldier::Point p, const soldier::Game& g) {
    return std::clamp(static_cast<float>(p.x - g.player().x) / 12.0f, -0.8f, 0.8f);
}

// Distance (cells) from the player to the nearest guillotine, or a large number.
float nearest_guillotine(const soldier::Game& g, float& pan) {
    const soldier::Point pl = g.player();
    float best = 1e9f;
    const int r = 12;
    for (int y = std::max(0, pl.y - r); y <= std::min(g.height() - 1, pl.y + r); ++y)
        for (int x = std::max(0, pl.x - r); x <= std::min(g.width() - 1, pl.x + r); ++x) {
            const int code = static_cast<int>(g.cell({x, y}));
            if (code != 48 && !(code >= 49 && code <= 52)) continue;
            const float d = std::hypot(static_cast<float>(x - pl.x), static_cast<float>(y - pl.y));
            if (d < best) {
                best = d;
                pan = std::clamp(static_cast<float>(x - pl.x) / 12.0f, -0.8f, 0.8f);
            }
        }
    return best;
}

}  // namespace

void EventSounds::add(double at, std::string name, float volume, float pitch, float pan) {
    queue_.push_back({at, std::move(name), volume, pitch, pan});
}

void EventSounds::on_event(const soldier::Event& e, const soldier::Game& g, const LevelView& view, const ViewClock& c) {
    const double now = c.now;
    switch (e.type) {
    case EventType::Moved:
        add(now, (steps_++ & 1) ? "step_2" : "step", 0.5f);
        break;
    case EventType::Turned: add(now, "turn", 0.45f); break;
    case EventType::Bumped: add(now, "turn", 0.2f, 0.7f); break;
    case EventType::BoxPushed: add(now, "push_box", 0.7f, 1.0f, pan_of(e.to, g)); break;
    case EventType::BoxSank: add(view.box_arrival(e.at), "box_sink", 0.8f, 1.0f, pan_of(e.at, g)); break;
    case EventType::ButtonPressed: {
        double t = view.box_arrival(e.at);
        if (t < now) t = g.player() == e.at ? view.step_end(now) : now;
        add(t, "button_press", 0.7f, 1.0f, pan_of(e.at, g));
        break;
    }
    case EventType::ButtonReleased: add(now, "button_release", 0.6f, 1.0f, pan_of(e.at, g)); break;
    case EventType::DoorsOpened: add(now, "door_open", 0.8f); break;
    case EventType::DoorsClosed: add(now, "door_close", 0.8f); break;
    case EventType::KeyPicked: add(now + (view.step_end(now) - now) * 0.5, "key_pickup", 0.8f); break;
    case EventType::KeyDropped: add(now, "key_drop", 0.8f, 1.0f, pan_of(e.at, g)); break;
    case EventType::ColorDoorUnlocked: add(now, "door_unlock", 0.9f, 1.0f, pan_of(e.at, g)); break;
    case EventType::WallRevealed: add(now, "wall_reveal", 0.8f, 1.0f, pan_of(e.at, g)); break;
    case EventType::FakeWallEntered: add(now, "wall_reveal", 0.35f, 1.4f); break;
    case EventType::ItemSelected: add(now, "menu_move", 0.6f); break;
    case EventType::GuillotinesClosed: {
        float pan = 0;
        const float d = nearest_guillotine(g, pan);
        if (d < 12.0f) add(now, "guillotine_chop", 0.55f * (1.0f - d / 13.0f), 1.0f, pan);
        break;
    }
    case EventType::TimerChanged:
        if (e.value <= 15 && e.value >= 0) add(now, "timer_tick", 0.6f);
        break;
    case EventType::Died: {
        const double t = view.step_end(now);
        switch (e.cause) {
        case soldier::DeathCause::GaveUp: add(t, "give_up", 0.9f); break;
        case soldier::DeathCause::Drowned: add(t, "drown", 0.9f); break;
        case soldier::DeathCause::Guillotine:
            add(t, "guillotine_chop", 0.9f);
            add(t + 0.1, "death", 0.8f);
            break;
        case soldier::DeathCause::TimeOut: add(t, "death", 0.9f); break;
        }
        break;
    }
    case EventType::Won: add(view.step_end(now), "level_win", 0.9f); break;
    default: break;
    }
}

void EventSounds::update(Resources& res, double now) {
    for (std::size_t i = 0; i < queue_.size();) {
        if (queue_[i].at <= now) {
            const Cue c = queue_[i];
            queue_.erase(queue_.begin() + static_cast<std::ptrdiff_t>(i));
            res.play(c.name, c.volume, c.pitch, c.pan);
        } else {
            ++i;
        }
    }
}

}  // namespace sg
