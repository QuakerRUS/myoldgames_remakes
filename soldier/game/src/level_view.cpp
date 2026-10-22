#include "level_view.hpp"

#include <algorithm>
#include <cmath>
#include <string>

#include "resources.hpp"

namespace sg {

using soldier::Dir;
using soldier::EventType;
using soldier::Point;
using soldier::Tile;

namespace {

constexpr double kTurnTime = 0.14;
constexpr double kBumpTime = 0.14;
constexpr double kPickupTime = 0.35;
constexpr double kDropTime = 0.25;
constexpr double kColorDoorFade = 0.3;
constexpr double kWinTime = 0.8;

vs::Vec2 cell_pos(Point p) { return {static_cast<float>(p.x) * kCell, static_cast<float>(p.y) * kCell}; }

float smooth(float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    return t * t * (3 - 2 * t);
}

vs::DrawParams scaled(const vs::Camera2D& cam, uint8_t alpha = 255) {
    vs::DrawParams p;
    p.scale = {cam.zoom, cam.zoom};
    p.tint = vs::Color{255, 255, 255, alpha};
    return p;
}

}  // namespace

LevelView::LevelView(const soldier::Game& game) : game_(&game) {
    doors_opening_ = game.doors_open();
}

Tile LevelView::at(int x, int y) const {
    if (x < 0 || y < 0 || x >= game_->width() || y >= game_->height()) return Tile::Wall;
    return game_->cell({x, y});
}

TileArt LevelView::art_at(int x, int y) const {
    const Tile t = at(x, y);
    if (death_cell_ && *death_cell_ == Point{x, y}) return death_cell_art(t, death_ground_);
    return tile_art(t);
}

double LevelView::step_duration(const ViewClock& c) const {
    const int ticks = std::max(1, game_->move_ready_tick() - game_->tick_count());
    const double d = ticks * c.tick_len - c.tick_elapsed;
    return std::clamp(d, c.tick_len * 0.4, c.tick_len * soldier::kMoveCooldown);
}

double LevelView::step_end(double now) const {
    if (motion_ && motion_->start + motion_->dur > now) return motion_->start + motion_->dur;
    return now;
}

double LevelView::box_arrival(Point at) const {
    for (const BoxMotion& b : boxes_)
        if (b.to == at) return b.start + b.dur;
    return 0;
}

const LevelView::BoxMotion* LevelView::box_motion_to(Point p, double now) const {
    for (const BoxMotion& b : boxes_) {
        if (!(b.to == p)) continue;
        if (now < b.start + b.dur) return &b;
    }
    return nullptr;
}

bool LevelView::hide_static_object(Point p, double now) const {
    if (box_motion_to(p, now)) return true;
    for (const Effect& e : effects_)
        if (e.kind == FxKind::KeyDrop && e.at == p && now < e.start + kDropTime) return true;
    return false;
}

// ---- events ------------------------------------------------------------------------------

void LevelView::on_event(const soldier::Event& e, const ViewClock& c) {
    const double now = c.now;
    switch (e.type) {
    case EventType::Moved: {
        Motion m;
        m.from = e.at;
        m.to = e.to;
        m.start = now;
        m.dur = step_duration(c);
        // A push in this batch: BoxPushed came right before with the same start time.
        m.push = !boxes_.empty() && boxes_.back().start == now && boxes_.back().from == e.to;
        m.parity = steps_++ & 1;
        motion_ = m;
        idle_since_ = now + m.dur;
        break;
    }
    case EventType::Turned:
        turn_start_ = std::max(now, step_end(now));
        idle_since_ = turn_start_ + kTurnTime;
        break;
    case EventType::Bumped:
        bump_start_ = now;
        bump_dir_ = e.dir;
        break;
    case EventType::BoxPushed: {
        // Drop finished motions, then start this one (same duration as the player's step).
        boxes_.erase(std::remove_if(boxes_.begin(), boxes_.end(),
                                    [&](const BoxMotion& b) { return now > b.start + b.dur + 1.0; }),
                     boxes_.end());
        BoxMotion b;
        b.from = e.at;
        b.to = e.to;
        b.start = now;
        b.dur = step_duration(c);
        boxes_.push_back(b);
        effects_.push_back({FxKind::Dust, e.at, now});
        break;
    }
    case EventType::BoxSank:
        for (BoxMotion& b : boxes_)
            if (b.to == e.at && b.start == now) b.sinks = true;
        break;
    case EventType::ButtonPressed: {
        double t = now;
        if (const double arr = box_arrival(e.at); arr > now) t = arr;
        else if (game_->player() == e.at) t = now + step_duration(c);
        buttons_[index(e.at)] = {t, true};
        last_press_time_ = t;
        break;
    }
    case EventType::ButtonReleased:
        buttons_[index(e.at)] = {now, false};
        break;
    case EventType::DoorsOpened:
        doors_opening_ = true;
        doors_start_ = std::max(now, last_press_time_);
        break;
    case EventType::DoorsClosed:
        doors_opening_ = false;
        doors_start_ = now;
        break;
    case EventType::KeyPicked:
        effects_.push_back({FxKind::KeyPickup, e.at, now + step_duration(c) * 0.5, e.color});
        break;
    case EventType::KeyDropped:
        effects_.push_back({FxKind::KeyDrop, e.at, now, e.color});
        break;
    case EventType::ColorDoorUnlocked:
        effects_.push_back({FxKind::ColorDoor, e.at, now, e.color});
        break;
    case EventType::WallRevealed:
        effects_.push_back({FxKind::Reveal, e.at, now});
        break;
    case EventType::Died:
        death_cell_ = e.at;
        death_tile_ = e.tile;
        death_ground_ = game_->ground();
        death_start_ = step_end(now);
        break;
    case EventType::Won:
        win_start_ = step_end(now);
        break;
    default:
        break;
    }
    // Forget old effects.
    effects_.erase(std::remove_if(effects_.begin(), effects_.end(), [&](const Effect& f) { return now > f.start + 3.0; }),
                   effects_.end());
}

// ---- queries -----------------------------------------------------------------------------

vs::Vec2 LevelView::player_center(double now) const {
    vs::Vec2 p = cell_pos(game_->player());
    if (motion_ && now < motion_->start + motion_->dur) {
        const float t = static_cast<float>((now - motion_->start) / motion_->dur);
        p = vs::lerp(cell_pos(motion_->from), cell_pos(motion_->to), std::clamp(t, 0.0f, 1.0f));
    }
    return p + vs::Vec2{kCell / 2, kCell / 2};
}

bool LevelView::ending_done(double now) const {
    if (game_->status() == soldier::Status::Dead) {
        const double len = 1.6;  // the longest death animation is about 1.5 s
        return now > death_start_ + len;
    }
    if (game_->status() == soldier::Status::Won && win_start_) return now > *win_start_ + kWinTime;
    return false;
}

// ---- drawing -----------------------------------------------------------------------------

void LevelView::draw_ground(vs::Renderer& r, const Resources& res, const vs::Camera2D& cam, int x, int y,
                            const TileArt& art, const ViewClock& c) const {
    const vs::SpriteSheet& t = res.tiles;
    const vs::Vec2 pos = cam.world_to_screen(cell_pos({x, y}));
    const vs::DrawParams dp = scaled(cam);
    const double ms = c.now * 1000.0;
    const Point p{x, y};
    GroundArt ground = art.ground;

    // A box still sliding into water: show the water until it has sunk.
    for (const BoxMotion& b : boxes_)
        if (b.sinks && b.to == p && c.now < b.start + b.dur + anim_length(t.animation("box_sink")) / 1000.0)
            ground = GroundArt::Water;

    auto floor = [&] {
        const unsigned h = cell_hash(x, y) % 16;
        const int v = h < 13 ? 0 : h < 14 ? 1 : h < 15 ? 2 : 3;
        t.draw(r, "floor_" + std::to_string(v), pos, dp);
    };

    switch (ground) {
    case GroundArt::Floor: floor(); break;
    case GroundArt::Wall: {
        int mask = 0;
        if (at(x, y - 1) == Tile::Wall) mask |= 1;
        if (at(x + 1, y) == Tile::Wall) mask |= 2;
        if (at(x, y + 1) == Tile::Wall) mask |= 4;
        if (at(x - 1, y) == Tile::Wall) mask |= 8;
        t.draw(r, "wall_" + std::to_string(mask), pos, dp);
        break;
    }
    case GroundArt::DarkWall: {
        // Outside the level counts as a plain wall (at() returns Wall there): plain walls join
        // the map edge, dark walls do not. The editor's TilePainter uses the same rule.
        auto dark = [&](int ax, int ay) {
            const Tile n = at(ax, ay);
            return n == Tile::DarkWall || n == Tile::FakeWall;
        };
        int mask = 0;
        if (dark(x, y - 1)) mask |= 1;
        if (dark(x + 1, y)) mask |= 2;
        if (dark(x, y + 1)) mask |= 4;
        if (dark(x - 1, y)) mask |= 8;
        t.draw(r, "darkwall_" + std::to_string(mask), pos, dp);
        break;
    }
    case GroundArt::Exit: t.draw(r, frame_at(t.animation("exit"), ms), pos, dp); break;
    case GroundArt::Button: {
        const bool pressed = art.object != ObjectArt::None || (game_->player() == p && game_->status() != soldier::Status::Dead);
        std::string frame = pressed ? "button_down" : "button_up";
        if (const auto it = buttons_.find(index(p)); it != buttons_.end()) {
            const double since = c.now - it->second.start;
            if (since < 0)
                frame = it->second.press ? "button_up" : "button_down";
            else
                frame = frame_at(t.animation(it->second.press ? "button_press" : "button_release"), since * 1000.0);
        }
        floor();
        t.draw(r, frame, pos, dp);
        break;
    }
    case GroundArt::Door: {
        std::string frame;
        const double since = c.now - doors_start_;
        if (since < 0)
            frame = doors_opening_ ? "door_0" : "door_4";
        else
            frame = frame_at(t.animation(doors_opening_ ? "door_open" : "door_close"), since * 1000.0);
        // A door with something inside is drawn open (the barrier cannot rise through it).
        if (art.object != ObjectArt::None && !doors_opening_ && since > 0.4) frame = "door_4";
        t.draw(r, frame, pos, dp);
        break;
    }
    case GroundArt::ColorDoor:
        t.draw(r, std::string("door_") + color_name(art.color) + "_0", pos, dp);
        break;
    case GroundArt::Guillotine: t.draw(r, "guillotine_base", pos, dp); break;
    case GroundArt::Water: t.draw(r, frame_at(t.animation("water"), ms), pos, dp); break;
    case GroundArt::SunkBox: t.draw(r, frame_at(t.animation("sunk_box"), ms), pos, dp); break;
    }
}

void LevelView::draw_ground_overlays(vs::Renderer& r, const Resources& res, const vs::Camera2D& cam, int x, int y,
                                     const TileArt& art) const {
    const vs::SpriteSheet& t = res.tiles;
    const vs::Vec2 pos = cam.world_to_screen(cell_pos({x, y}));
    const vs::DrawParams dp = scaled(cam);
    if (art.ground == GroundArt::Wall || art.ground == GroundArt::DarkWall) return;
    if (art.ground == GroundArt::Water || art.ground == GroundArt::SunkBox) {
        if (!is_water_like(at(x, y - 1))) t.draw(r, "water_edge_n", pos, dp);
        if (!is_water_like(at(x + 1, y))) t.draw(r, "water_edge_e", pos, dp);
        if (!is_water_like(at(x, y + 1))) t.draw(r, "water_edge_s", pos, dp);
        if (!is_water_like(at(x - 1, y))) t.draw(r, "water_edge_w", pos, dp);
    }
    const bool n = is_wall_like(at(x, y - 1)), w = is_wall_like(at(x - 1, y));
    if (n) t.draw(r, "shadow_n", pos, dp);
    if (w) t.draw(r, "shadow_w", pos, dp);
    if (!n && !w && is_wall_like(at(x - 1, y - 1))) t.draw(r, "shadow_nw", pos, dp);
}

void LevelView::draw_object(vs::Renderer& r, const Resources& res, const vs::Camera2D& cam, int x, int y,
                            const TileArt& art, const ViewClock& c) const {
    const Point p{x, y};
    const vs::Vec2 pos = cam.world_to_screen(cell_pos(p));
    const vs::DrawParams dp = scaled(cam);
    const bool is_player_cell = p == game_->player();
    switch (art.object) {
    case ObjectArt::None: break;
    case ObjectArt::Box:
        if (!hide_static_object(p, c.now)) res.tiles.draw(r, "box", pos, dp);
        break;
    case ObjectArt::Key:
        if (!hide_static_object(p, c.now)) res.tiles.draw(r, std::string("key_") + color_name(art.color), pos, dp);
        break;
    case ObjectArt::Player:
        // The live player is drawn separately (smoothly); other player tiles are decorations.
        if (!is_player_cell) res.player.draw(r, std::string("idle_") + dir_letter(art.facing) + "_0", pos, dp);
        break;
    case ObjectArt::Death:
        if (death_cell_ && *death_cell_ == p) break;  // the live death animation
        res.player.draw(r, frame_at(res.player.animation("death_" + std::to_string(static_cast<int>(art.death))), 1e9),
                        pos, dp);
        break;
    }
}

void LevelView::draw_player(vs::Renderer& r, const Resources& res, const vs::Camera2D& cam, double now) const {
    const vs::DrawParams dp = scaled(cam);
    if (death_cell_ && now >= death_start_) {
        const vs::Vec2 pos = cam.world_to_screen(cell_pos(*death_cell_));
        const std::string anim = "death_" + std::to_string(static_cast<int>(death_tile_));
        res.player.draw(r, frame_at(res.player.animation(anim), (now - death_start_) * 1000.0), pos, dp);
        return;
    }
    const char d = dir_letter(game_->facing());
    vs::Vec2 world = cell_pos(game_->player());
    std::string frame;
    if (motion_ && now < motion_->start + motion_->dur) {
        const float t = std::clamp(static_cast<float>((now - motion_->start) / motion_->dur), 0.0f, 1.0f);
        world = vs::lerp(cell_pos(motion_->from), cell_pos(motion_->to), t);
        const int half = t < 0.5f ? 0 : 1;
        if (motion_->push)
            frame = std::string("push_") + d + "_" + std::to_string(half);
        else
            frame = std::string("walk_") + d + "_" + std::to_string(motion_->parity * 2 + half);
    } else if (now - turn_start_ < kTurnTime && now >= turn_start_) {
        frame = frame_at(res.player.animation(std::string("turn_") + d), (now - turn_start_) * 1000.0);
    } else {
        frame = frame_at(res.player.animation(std::string("idle_") + d), (now - idle_since_) * 1000.0);
    }
    if (now - bump_start_ < kBumpTime && now >= bump_start_) {
        const auto dl = soldier::delta(bump_dir_);
        const float k = std::sin(static_cast<float>((now - bump_start_) / kBumpTime) * 3.14159f) * 3.0f;
        world += vs::Vec2{static_cast<float>(dl.dx) * k, static_cast<float>(dl.dy) * k};
    }
    uint8_t alpha = 255;
    if (win_start_ && now > *win_start_) {
        // Victory: the soldier is beamed away from the exit pad.
        const float t = std::clamp(static_cast<float>((now - *win_start_) / kWinTime), 0.0f, 1.0f);
        alpha = static_cast<uint8_t>(255 * (1 - t));
        world.y -= 10.0f * smooth(t);
        const vs::Vec2 c = cam.world_to_screen(cell_pos(game_->player()) + vs::Vec2{kCell / 2, kCell / 2});
        r.fill_circle(c, (6 + 18 * t) * cam.zoom, vs::Color{150, 255, 170, static_cast<uint8_t>(110 * (1 - t))});
    }
    res.player.draw(r, frame, cam.world_to_screen(world), scaled(cam, alpha));
    (void)dp;
}

void LevelView::draw_effects(vs::Renderer& r, const Resources& res, const vs::Camera2D& cam, double now,
                             bool ground) const {
    for (const Effect& e : effects_) {
        const vs::Vec2 pos = cam.world_to_screen(cell_pos(e.at));
        const double since = now - e.start;
        switch (e.kind) {
        case FxKind::ColorDoor: {
            if (!ground) break;
            const vs::AnimationDef* a = res.tiles.animation(std::string("door_") + color_name(e.color) + "_open");
            const double len = anim_length(a) / 1000.0;
            if (since > len + kColorDoorFade) break;
            const double fade = since > len ? 1.0 - (since - len) / kColorDoorFade : 1.0;
            res.tiles.draw(r, frame_at(a, std::max(0.0, since) * 1000.0), pos,
                           scaled(cam, static_cast<uint8_t>(255 * std::clamp(fade, 0.0, 1.0))));
            break;
        }
        case FxKind::KeyPickup: {
            if (ground) break;
            const std::string frame = std::string("key_") + color_name(e.color);
            if (since < 0) {
                res.tiles.draw(r, frame, pos, scaled(cam));
            } else if (since < kPickupTime) {
                const float t = static_cast<float>(since / kPickupTime);
                res.tiles.draw(r, frame, pos + vs::Vec2{0, -14.0f * smooth(t) * cam.zoom},
                               scaled(cam, static_cast<uint8_t>(255 * (1 - t))));
            }
            break;
        }
        case FxKind::KeyDrop: {
            if (ground || since < 0 || since >= kDropTime) break;
            const float t = static_cast<float>(since / kDropTime);
            const float hop = std::abs(std::cos(t * 3.14159f * 1.5f)) * (1 - t) * 10.0f;
            res.tiles.draw(r, std::string("key_") + color_name(e.color), pos + vs::Vec2{0, -hop * cam.zoom}, scaled(cam));
            break;
        }
        case FxKind::Reveal:
            if (ground || since < 0) break;
            if (const vs::AnimationDef* a = res.tiles.animation("reveal_fx"); since * 1000.0 < anim_length(a))
                res.tiles.draw(r, frame_at(a, since * 1000.0), pos, scaled(cam));
            break;
        case FxKind::Dust:
            if (ground || since < 0) break;
            if (const vs::AnimationDef* a = res.tiles.animation("dust"); since * 1000.0 < anim_length(a))
                res.tiles.draw(r, frame_at(a, since * 1000.0), pos, scaled(cam, 200));
            break;
        }
    }
}

void LevelView::render(vs::Renderer& r, const Resources& res, const vs::Camera2D& cam, const ViewClock& c) const {
    const int w = game_->width(), h = game_->height();
    r.set_clip(cam.viewport);
    // Outside the level: dark concrete pit with a soft edge around the field.
    r.fill_gradient(cam.viewport, vs::rgb(0x10141A), vs::rgb(0x07090C));
    const vs::Vec2 tl = cam.world_to_screen({0, 0});
    const vs::Vec2 br = cam.world_to_screen({static_cast<float>(w) * kCell, static_cast<float>(h) * kCell});
    for (int i = 1; i <= 3; ++i) {
        const float d = static_cast<float>(i) * 4.0f;
        r.fill_rounded_rect({tl.x - d + 6, tl.y - d + 8, br.x - tl.x + d * 2, br.y - tl.y + d * 2}, d * 1.5f,
                            vs::Color{0, 0, 0, 28});
    }

    const vs::RectF view = cam.view_rect();
    const int x0 = std::max(0, static_cast<int>(std::floor(view.x / kCell)) - 1);
    const int y0 = std::max(0, static_cast<int>(std::floor(view.y / kCell)) - 1);
    const int x1 = std::min(w - 1, static_cast<int>(std::ceil(view.right() / kCell)) + 1);
    const int y1 = std::min(h - 1, static_cast<int>(std::ceil(view.bottom() / kCell)) + 1);

    // 1. ground, 2. shadows / water edges / door effects
    for (int y = y0; y <= y1; ++y)
        for (int x = x0; x <= x1; ++x) draw_ground(r, res, cam, x, y, art_at(x, y), c);
    for (int y = y0; y <= y1; ++y)
        for (int x = x0; x <= x1; ++x) draw_ground_overlays(r, res, cam, x, y, art_at(x, y));
    draw_effects(r, res, cam, c.now, true);

    // 3. static objects
    for (int y = y0; y <= y1; ++y)
        for (int x = x0; x <= x1; ++x) draw_object(r, res, cam, x, y, art_at(x, y), c);

    // 4. moving boxes (and boxes sinking into water)
    const vs::AnimationDef* sink = res.tiles.animation("box_sink");
    for (const BoxMotion& b : boxes_) {
        const double end = b.start + b.dur;
        if (c.now < end) {
            const float t = std::clamp(static_cast<float>((c.now - b.start) / b.dur), 0.0f, 1.0f);
            res.tiles.draw(r, "box", cam.world_to_screen(vs::lerp(cell_pos(b.from), cell_pos(b.to), t)), scaled(cam));
        } else if (b.sinks && (c.now - end) * 1000.0 < anim_length(sink)) {
            res.tiles.draw(r, frame_at(sink, (c.now - end) * 1000.0), cam.world_to_screen(cell_pos(b.to)), scaled(cam));
        }
    }

    // 5. the player
    if (game_->status() != soldier::Status::Won || !win_start_ || c.now < *win_start_ + kWinTime) draw_player(r, res, cam, c.now);

    // 6. guillotine tops (synchronised with the logic's part of the second), shimmer, effects
    std::string top = "guillotine_top_0";
    if (game_->part() == 4)
        top = frame_at(res.tiles.animation("guillotine_top_close"), c.tick_elapsed * 1000.0);
    else if (game_->part() == 1 && game_->tick_count() > 0)
        top = frame_at(res.tiles.animation("guillotine_top_open"), c.tick_elapsed * 1000.0);
    const vs::AnimationDef* shimmer = res.tiles.animation("darkwall_shimmer");
    for (int y = y0; y <= y1; ++y)
        for (int x = x0; x <= x1; ++x) {
            const Tile t = at(x, y);
            const vs::Vec2 pos = cam.world_to_screen(cell_pos({x, y}));
            const TileArt art = art_at(x, y);
            if (art.ground == GroundArt::Guillotine && t != Tile::DeathGuillotine &&
                !(death_cell_ && *death_cell_ == Point{x, y} && c.now >= death_start_))
                res.tiles.draw(r, top, pos, scaled(cam));
            if (art.ground == GroundArt::DarkWall)
                res.tiles.draw(r, frame_at(shimmer, c.now * 1000.0 + static_cast<double>(cell_hash(x, y) % 4000)), pos,
                               scaled(cam, 170));
        }
    draw_effects(r, res, cam, c.now, false);
    r.set_clip(std::nullopt);
}

}  // namespace sg
