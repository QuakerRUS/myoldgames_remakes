#pragma once
// Factories of all Piton scenes. Each scene receives the shared state (assets, save, theme);
// the first scene may receive nullptr and creates it in on_enter.

#include <memory>
#include <string>

#include <vs/engine/scene.hpp>

#include "assets.hpp"

namespace piton_game {

enum class PlayMode { Play, Demo, Test };

struct GameSetup {
    PlayMode mode = PlayMode::Play;
    int level = 1;           // 1..10 (ignored for Test)
    int score = 0;           // carried over from the previous levels
    std::string test_path;   // PlayMode::Test: the level file
    // High score when this game started (carried over to the next levels): the GAME OVER /
    // win "NEW HIGH SCORE!" needs score > this. -1 = take save.high_score when the scene starts.
    int high_at_start = -1;
};

enum class MenuPage { Main, Levels, Options };

std::unique_ptr<vs::Scene> make_intro_scene(SharedPtr shared);
std::unique_ptr<vs::Scene> make_menu_scene(SharedPtr shared, MenuPage page = MenuPage::Main);
std::unique_ptr<vs::Scene> make_game_scene(SharedPtr shared, GameSetup setup);
// previous_high: the high score before this game (-1 = save.high_score when the scene starts).
std::unique_ptr<vs::Scene> make_win_scene(SharedPtr shared, int score, int previous_high = -1);

}  // namespace piton_game
