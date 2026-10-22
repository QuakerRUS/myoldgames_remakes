#include "tile_art.hpp"

#include <cmath>

namespace sg {

using soldier::Tile;

GroundArt ground_art(soldier::Ground g) {
    switch (g) {
    case soldier::Ground::Floor: return GroundArt::Floor;
    case soldier::Ground::Button: return GroundArt::Button;
    case soldier::Ground::Door: return GroundArt::Door;
    case soldier::Ground::Guillotine: return GroundArt::Guillotine;
    case soldier::Ground::SunkBox: return GroundArt::SunkBox;
    }
    return GroundArt::Floor;
}

TileArt death_cell_art(Tile t, soldier::Ground ground) {
    TileArt a = tile_art(t);
    if (a.object == ObjectArt::Death && a.ground == GroundArt::Floor) a.ground = ground_art(ground);
    return a;
}

TileArt tile_art(Tile t) {
    TileArt a;
    const int code = static_cast<int>(t);
    if (const auto g = soldier::player_ground(t)) {
        a.object = ObjectArt::Player;
        a.facing = *soldier::player_facing(t);
        a.ground = ground_art(*g);
        return a;
    }
    if (code >= 21 && code <= 44) {
        a.color = *soldier::key_color(t);
        switch ((code - 21) % 4) {
        case 0: a.ground = GroundArt::Floor; a.object = ObjectArt::Key; break;
        case 1: a.ground = GroundArt::ColorDoor; break;
        case 2: a.ground = GroundArt::Button; a.object = ObjectArt::Key; break;
        default: a.ground = GroundArt::Door; a.object = ObjectArt::Key; break;
        }
        return a;
    }
    switch (t) {
    case Tile::Wall: a.ground = GroundArt::Wall; break;
    case Tile::Exit: a.ground = GroundArt::Exit; break;
    case Tile::DeathGaveUp:
    case Tile::DeathTimeOut: a.object = ObjectArt::Death; a.death = t; break;
    case Tile::Box: a.object = ObjectArt::Box; break;
    case Tile::Button: a.ground = GroundArt::Button; break;
    case Tile::BoxOnButton: a.ground = GroundArt::Button; a.object = ObjectArt::Box; break;
    case Tile::Door: a.ground = GroundArt::Door; break;
    case Tile::BoxInDoor: a.ground = GroundArt::Door; a.object = ObjectArt::Box; break;
    case Tile::DarkWall:
    case Tile::FakeWall: a.ground = GroundArt::DarkWall; break;
    case Tile::Guillotine: a.ground = GroundArt::Guillotine; break;
    case Tile::DeathGuillotine: a.ground = GroundArt::Guillotine; a.object = ObjectArt::Death; a.death = t; break;
    case Tile::Water: a.ground = GroundArt::Water; break;
    case Tile::DeathDrowned: a.ground = GroundArt::Water; a.object = ObjectArt::Death; a.death = t; break;
    case Tile::SunkBox: a.ground = GroundArt::SunkBox; break;
    case Tile::DeathGaveUpOnSunkBox:
    case Tile::DeathTimeOutOnSunkBox: a.ground = GroundArt::SunkBox; a.object = ObjectArt::Death; a.death = t; break;
    case Tile::BoxOnSunkBox: a.ground = GroundArt::SunkBox; a.object = ObjectArt::Box; break;
    default: break;
    }
    return a;
}

bool is_wall_like(Tile t) { return t == Tile::Wall || t == Tile::DarkWall || t == Tile::FakeWall; }

bool is_water_like(Tile t) {
    return t == Tile::Water || t == Tile::DeathDrowned || t == Tile::SunkBox || t == Tile::BoxOnSunkBox ||
           t == Tile::DeathGaveUpOnSunkBox || t == Tile::DeathTimeOutOnSunkBox ||
           (static_cast<int>(t) >= 57 && static_cast<int>(t) <= 60);
}

char dir_letter(soldier::Dir d) {
    switch (d) {
    case soldier::Dir::Down: return 'd';
    case soldier::Dir::Up: return 'u';
    case soldier::Dir::Left: return 'l';
    case soldier::Dir::Right: return 'r';
    }
    return 'd';
}

const char* color_name(soldier::KeyColor c) {
    static const char* names[] = {"red", "green", "blue", "yellow", "white", "black"};
    return names[static_cast<int>(c)];
}

double anim_length(const vs::AnimationDef* def) { return def ? static_cast<double>(def->total_ms()) : 0.0; }

std::string frame_at(const vs::AnimationDef* def, double ms) {
    if (!def || def->frames.empty()) return {};
    const double total = anim_length(def);
    if (ms < 0) ms = 0;
    if (def->loop && total > 0)
        ms = std::fmod(ms, total);
    else if (ms >= total)
        return def->frames.back().frame;
    for (const auto& f : def->frames) {
        if (ms < f.ms) return f.frame;
        ms -= f.ms;
    }
    return def->frames.back().frame;
}

unsigned cell_hash(int x, int y) {
    unsigned h = static_cast<unsigned>(x) * 73856093u ^ static_cast<unsigned>(y) * 19349663u;
    h ^= h >> 13;
    h *= 0x5bd1e995u;
    h ^= h >> 15;
    return h;
}

}  // namespace sg
