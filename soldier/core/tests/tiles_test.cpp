#include <set>
#include <string>

#include "util.hpp"

using namespace soldier;

TEST_CASE("every tile code has a unique character and name") {
    std::set<char> chars;
    std::set<std::string> names;
    for (int i = 0; i < kTileCount; ++i) {
        const Tile t = static_cast<Tile>(i);
        const char c = tile_char(t);
        CHECK(c > ' ');
        CHECK(c != '"');
        CHECK(c != '\\');
        CHECK(tile_from_char(c) == t);
        chars.insert(c);
        names.insert(std::string(tile_name(t)));
    }
    CHECK(chars.size() == kTileCount);
    CHECK(names.size() == kTileCount);
    CHECK_FALSE(tile_from_char('Z').has_value());
}

TEST_CASE("player tiles: ground and facing") {
    CHECK(player_tile(Ground::Floor, Dir::Down) == Tile::PlayerDown);
    CHECK(player_tile(Ground::Button, Dir::Right) == Tile::PlayerOnButtonRight);
    CHECK(player_tile(Ground::Door, Dir::Up) == Tile::PlayerInDoorUp);
    CHECK(player_tile(Ground::Guillotine, Dir::Left) == Tile::PlayerInGuillotineLeft);
    CHECK(player_tile(Ground::SunkBox, Dir::Down) == Tile::PlayerOnSunkBoxDown);
    int players = 0;
    for (int i = 0; i < kTileCount; ++i) {
        const Tile t = static_cast<Tile>(i);
        if (!is_player(t)) continue;
        ++players;
        CHECK(player_tile(*player_ground(t), *player_facing(t)) == t);
    }
    CHECK(players == 20);
    CHECK_FALSE(is_player(Tile::Box));
}

TEST_CASE("turning follows subroutines 1800 (cw) and 1850 (ccw)") {
    // 1800: 3->5, 4->6, 5->4, 6->3  (down->left, up->right, left->up, right->down)
    CHECK(turn_cw(Dir::Down) == Dir::Left);
    CHECK(turn_cw(Dir::Up) == Dir::Right);
    CHECK(turn_cw(Dir::Left) == Dir::Up);
    CHECK(turn_cw(Dir::Right) == Dir::Down);
    // 1850: 3->6, 4->5, 5->3, 6->4
    CHECK(turn_ccw(Dir::Down) == Dir::Right);
    CHECK(turn_ccw(Dir::Up) == Dir::Left);
    CHECK(turn_ccw(Dir::Left) == Dir::Down);
    CHECK(turn_ccw(Dir::Right) == Dir::Up);
}

TEST_CASE("coloured codes") {
    CHECK(key_tile(KeyColor::Red) == Tile::RedKey);
    CHECK(color_door_tile(KeyColor::Black) == Tile::BlackDoor);
    CHECK(key_on_button_tile(KeyColor::Green) == Tile::GreenKeyOnButton);
    CHECK(key_in_door_tile(KeyColor::White) == Tile::WhiteKeyInDoor);
    CHECK(key_color(Tile::YellowKeyInDoor) == KeyColor::Yellow);
    CHECK_FALSE(key_color(Tile::BoxInDoor).has_value());
    CHECK_FALSE(key_color(Tile::DarkWall).has_value());
    for (Tile t : {Tile::DeathGaveUp, Tile::DeathTimeOut, Tile::DeathGuillotine, Tile::DeathDrowned,
                   Tile::DeathGaveUpOnSunkBox, Tile::DeathTimeOutOnSunkBox})
        CHECK(is_death(t));
}

TEST_CASE("command names round-trip") {
    for (Command c : {Command::move(Dir::Down), Command::move(Dir::Up), Command::move(Dir::Left),
                      Command::move(Dir::Right), Command::turn_cw(), Command::turn_ccw(), Command::next_item(),
                      Command::use_item(), Command::give_up()})
        CHECK(command_from_name(command_name(c)) == c);
    CHECK_FALSE(command_from_name("jump").has_value());
}
