// Port of GAME1.BAS 1.1. Subroutine numbers of the original are given in comments.
#include "soldier/game.hpp"

#include <utility>

namespace soldier {
namespace {

Point operator+(Point p, Delta d) { return {p.x + d.dx, p.y + d.dy}; }

}  // namespace

Game::Game(Level level) : level_(std::move(level)) {
    player_ = level_.start();
    const Tile t = level_.at(player_);
    ground_ = *player_ground(t);
    facing_ = *player_facing(t);
    for (Tile c : level_.cells)
        if (c == Tile::Button) ++onkey_;  // the original counted code 9 only
    timer_ = level_.time_limit > 0 ? level_.time_limit : -1;
}

std::vector<Event> Game::take_events() { return std::exchange(events_, {}); }

void Game::apply(Command c) {
    if (status_ != Status::Playing) return;
    if (c.type == CommandType::Move && !can_move_now()) {
        pending_ = c.dir;
        return;
    }
    pending_.reset();
    execute(c);
}

void Game::tick() {
    if (status_ != Status::Playing) return;
    ++tick_;
    if (part() == 4) {  // main loop, before the 4th part
        emit({.type = EventType::GuillotinesClosed});
        if (ground_ == Ground::Guillotine) {
            die(DeathCause::Guillotine);
            return;
        }
    } else if (part() == 1) {  // main loop, after the 4th part; then subroutine 1700
        emit({.type = EventType::GuillotinesOpened});
        if (timer_ == 0) {
            die(DeathCause::TimeOut);
            return;
        }
        if (timer_ > 0) {
            --timer_;
            emit({.type = EventType::TimerChanged, .value = timer_});
        }
    }
    if (pending_ && can_move_now()) {
        const Dir d = *pending_;
        pending_.reset();
        execute(Command::move(d));
    }
}

void Game::execute(Command c) {
    history_.push_back({tick_, c});
    const bool doors_before = doors_open();
    switch (c.type) {
    case CommandType::GiveUp: die(DeathCause::GaveUp); break;
    case CommandType::TurnCW: turn(true); break;
    case CommandType::TurnCCW: turn(false); break;
    case CommandType::NextItem:  // 1950 (unreachable in 1.1 because of a bug; works here)
        if (!items_.empty()) {
            cursor_ = (cursor_ + 1) % static_cast<int>(items_.size());
            emit({.type = EventType::ItemSelected, .value = cursor_});
        }
        break;
    case CommandType::UseItem:  // 1960
        if (!items_.empty()) use_key(items_[static_cast<std::size_t>(cursor_)], player_ + delta(facing_));
        break;
    case CommandType::Move: move(c.dir); break;
    }
    if (doors_before != doors_open())
        emit({.type = doors_open() ? EventType::DoorsOpened : EventType::DoorsClosed});
}

void Game::turn(bool cw) {  // 1800 / 1850
    set_facing(cw ? turn_cw(facing_) : turn_ccw(facing_));
}

void Game::set_facing(Dir d) {
    facing_ = d;
    put_player();
    emit({.type = EventType::Turned, .dir = d});
}

void Game::put_player() { level_.at(player_) = player_tile(ground_, facing_); }

void Game::leave() {  // 12100
    level_.at(player_) = ground_tile(ground_);
    if (ground_ == Ground::Button) {
        ++onkey_;
        emit({.type = EventType::ButtonReleased, .at = player_});
    }
}

void Game::step(Point to, Ground g, Dir d) {
    const Point from = player_;
    leave();
    player_ = to;
    ground_ = g;
    facing_ = d;
    put_player();
    emit({.type = EventType::Moved, .at = from, .to = to, .dir = d});
    move_ready_tick_ = tick_ + kMoveCooldown;  // p& = gs& * 2
}

void Game::bump(Point target, Dir d) {  // 2100; also turns on a sunk box (the original did not)
    emit({.type = EventType::Bumped, .at = target, .dir = d});
    if (facing_ != d) set_facing(d);
}

void Game::press_button(Point p) {
    --onkey_;
    emit({.type = EventType::ButtonPressed, .at = p});
}

bool Game::can_push(Point box, Dir d, bool into_water) const {  // checks of 2300 / 2500 / 2700 / 5600
    const Point beyond = box + delta(d);
    if (!in_bounds(beyond)) return false;
    switch (level_.at(beyond)) {
    case Tile::Floor:
    case Tile::Button: return true;
    case Tile::Door: return onkey_ == 0;
    case Tile::Water:
    case Tile::SunkBox: return into_water;  // 2300/5600 checked ml(absx - 2) here for Right: fixed
    default: return false;
    }
}

void Game::push_box(Point box, Dir d) {  // 12200
    const Point beyond = box + delta(d);
    Tile& t = level_.at(beyond);
    emit({.type = EventType::BoxPushed, .at = box, .to = beyond, .dir = d});
    switch (t) {
    case Tile::Floor: t = Tile::Box; break;
    case Tile::Button:
        t = Tile::BoxOnButton;
        press_button(beyond);
        break;
    case Tile::Door: t = Tile::BoxInDoor; break;
    case Tile::Water:
        t = Tile::SunkBox;
        emit({.type = EventType::BoxSank, .at = beyond});
        break;
    case Tile::SunkBox: t = Tile::BoxOnSunkBox; break;
    default: break;
    }
}

bool Game::use_key(KeyColor c, Point target) {  // 20000..20500
    if (!in_bounds(target)) return false;
    Tile& t = level_.at(target);
    if (t == Tile::Floor) {
        t = key_tile(c);
    } else if (t == Tile::Button) {
        t = key_on_button_tile(c);
        press_button(target);
    } else if (t == Tile::Door && onkey_ == 0) {
        t = key_in_door_tile(c);
    } else if (t == color_door_tile(c)) {
        t = Tile::Floor;
        remove_item(cursor_);
        emit({.type = EventType::ColorDoorUnlocked, .at = target, .color = c});
        return true;
    } else {
        return false;
    }
    remove_item(cursor_);
    emit({.type = EventType::KeyDropped, .at = target, .color = c, .tile = t});
    return true;
}

void Game::remove_item(int index) {  // 12300
    items_.erase(items_.begin() + index);
    cursor_ = 0;
}

void Game::die(DeathCause c) {
    Tile t = Tile::DeathGaveUp;
    switch (c) {
    case DeathCause::GaveUp:
        t = ground_ == Ground::Guillotine ? Tile::DeathGuillotine
            : ground_ == Ground::SunkBox  ? Tile::DeathGaveUpOnSunkBox
                                          : Tile::DeathGaveUp;
        break;
    case DeathCause::TimeOut:
        t = ground_ == Ground::Guillotine ? Tile::DeathGuillotine
            : ground_ == Ground::SunkBox  ? Tile::DeathTimeOutOnSunkBox
                                          : Tile::DeathTimeOut;
        break;
    case DeathCause::Guillotine: t = Tile::DeathGuillotine; break;
    case DeathCause::Drowned: t = Tile::DeathDrowned; break;
    }
    status_ = Status::Dead;
    cause_ = c;
    death_tile_ = t;
    pending_.reset();
    level_.at(player_) = t;
    emit({.type = EventType::Died, .at = player_, .cause = c, .tile = t});
}

void Game::move(Dir d) {  // 1500
    const Point target = player_ + delta(d);
    const Tile t = in_bounds(target) ? level_.at(target) : Tile::Wall;  // outside = wall (fixed)
    if (const auto color = key_color(t)) {
        const int kind = (static_cast<int>(t) - 21) % 4;  // 0 key, 1 door, 2 key on button, 3 key in door
        if (kind == 1) {  // 2900 ...: the first key of that colour opens the door
            int found = -1;
            for (std::size_t i = 0; i < items_.size() && found < 0; ++i)
                if (items_[i] == *color) found = static_cast<int>(i);
            if (found < 0) return bump(target, d);
            cursor_ = found;
            use_key(*color, target);
            return step(target, Ground::Floor, d);
        }
        if (static_cast<int>(items_.size()) == kInventorySize) return bump(target, d);
        items_.push_back(*color);
        emit({.type = EventType::KeyPicked, .at = target, .color = *color});
        return step(target, kind == 0 ? Ground::Floor : kind == 2 ? Ground::Button : Ground::Door, d);
    }
    switch (t) {
    case Tile::Floor: return step(target, Ground::Floor, d);  // 2000
    case Tile::FakeWall:                                       // 2000 via 46
        emit({.type = EventType::FakeWallEntered, .at = target});
        return step(target, Ground::Floor, d);
    case Tile::Wall: return bump(target, d);
    case Tile::Exit: {  // 2200
        const Point from = player_;
        leave();
        player_ = target;
        facing_ = d;
        emit({.type = EventType::Moved, .at = from, .to = target, .dir = d});
        status_ = Status::Won;
        pending_.reset();
        emit({.type = EventType::Won, .at = target});
        return;
    }
    case Tile::Box:           // 2300
    case Tile::BoxOnSunkBox:  // 5600
        if (!can_push(target, d, true)) return bump(target, d);
        push_box(target, d);
        return step(target, t == Tile::Box ? Ground::Floor : Ground::SunkBox, d);
    case Tile::Button:  // 2400
        step(target, Ground::Button, d);
        return press_button(target);
    case Tile::BoxOnButton:  // 2500
    case Tile::BoxInDoor:    // 2700
        if (!can_push(target, d, false)) return bump(target, d);
        push_box(target, d);
        return step(target, t == Tile::BoxOnButton ? Ground::Button : Ground::Door, d);
    case Tile::Door:  // 2600
        if (onkey_ > 0) return bump(target, d);
        return step(target, Ground::Door, d);
    case Tile::DarkWall:  // 5200
        level_.at(target) = Tile::Wall;
        emit({.type = EventType::WallRevealed, .at = target});
        return bump(target, d);
    case Tile::Guillotine:  // 5300
        if (guillotines_closed()) return bump(target, d);
        return step(target, Ground::Guillotine, d);
    case Tile::Water: {  // 5400
        const Point from = player_;
        leave();
        player_ = target;
        facing_ = d;
        emit({.type = EventType::Moved, .at = from, .to = target, .dir = d});
        return die(DeathCause::Drowned);
    }
    case Tile::SunkBox: return step(target, Ground::SunkBox, d);  // 5500
    default:  // decorations (player-like and death tiles): nothing happens, no turn
        emit({.type = EventType::Bumped, .at = target, .dir = d});
        return;
    }
}

}  // namespace soldier
