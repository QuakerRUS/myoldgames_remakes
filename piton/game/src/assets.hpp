#pragma once
// Everything the scenes share: fonts, sprite sheets, images, sounds, music, the UI theme and
// the save data. One instance lives as long as any scene holds the shared_ptr (scenes die
// before the App, so the textures die before the renderer).

#include <memory>

#include <vs/engine/app.hpp>
#include <vs/engine/audio.hpp>
#include <vs/engine/font.hpp>
#include <vs/engine/sprite_sheet.hpp>
#include <vs/engine/texture.hpp>
#include <vs/engine/ui_theme.hpp>

#include "save_data.hpp"

namespace piton_game {

enum class Sfx { Eat, Turn, Die, LevelComplete, MenuMove, MenuSelect };
enum class Music { Menu, Game, Victory };

struct Assets {
    // Press Start 2P (pixel font, 8 px grid) and VT323.
    vs::Font font8, font16, font24, font32, font48;
    vs::Font vt32;

    vs::SpriteSheet tiles, snake, rabbit, ui;
    vs::Texture logo, menu_bg;

    vs::Sound eat, turn, die, level_complete, menu_move, menu_select;
    vs::Sound music_menu, music_game, music_victory;

    void load(vs::Renderer& r);
};

class Shared {
public:
    // Loads the assets and the save; applies the volumes.
    // Persists every fullscreen change (the menu, F11, Alt+Enter) into save.fullscreen.
    explicit Shared(vs::App& app);
    ~Shared();
    Shared(const Shared&) = delete;
    Shared& operator=(const Shared&) = delete;

    Assets assets;
    SaveData save;
    vs::Theme theme;  // vs::Menu / vs::Dialog look
    bool muted = false;  // fast-forward in screenshot mode: no sounds
    // The save had been edited and the progress was reset: the menu says so once.
    bool progress_reset_notice = false;

    void play(Sfx s, float volume = 1.0f, float pitch = 1.0f);
    void music(Music m, bool loop = true);
    void stop_music(double fade = 0.3);
    void apply_volumes();

    // Creates the shared state on first use (the first scene's on_enter).
    static std::shared_ptr<Shared> ensure(std::shared_ptr<Shared> existing, vs::App& app);

private:
    vs::App* app_;
};

using SharedPtr = std::shared_ptr<Shared>;

}  // namespace piton_game
