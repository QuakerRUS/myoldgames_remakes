// Everything the scenes share: graphics, fonts, sounds, music, the UI theme, the save data
// and the command line. Owned through std::shared_ptr by the scenes (so it dies with the
// last scene, before the App and its renderer).
#pragma once

#include <future>
#include <map>
#include <memory>
#include <string>

#include <vs/engine/app.hpp>
#include <vs/engine/audio.hpp>
#include <vs/engine/font.hpp>
#include <vs/engine/sprite_sheet.hpp>
#include <vs/engine/texture.hpp>
#include <vs/engine/ui_theme.hpp>

#include "cli.hpp"
#include "input_script.hpp"
#include "save.hpp"

namespace sg {

// Palette of the UI (matches the steel/hazard look of the sprites).
namespace colors {
inline constexpr vs::Color text = vs::rgb(0xD8DEE6);
inline constexpr vs::Color dim = vs::rgb(0x7C8794);
inline constexpr vs::Color accent = vs::rgb(0xFFC933);   // hazard yellow
inline constexpr vs::Color green = vs::rgb(0x6EE06A);
inline constexpr vs::Color yellow = vs::rgb(0xFFD84A);
inline constexpr vs::Color red = vs::rgb(0xFF5A4E);
inline constexpr vs::Color cyan = vs::rgb(0x7FD8FF);
inline constexpr vs::Color shadow = vs::rgba(0x000000C0);
}  // namespace colors

class Resources {
public:
    Resources(vs::App& app, CommandLine cmd);

    CommandLine cmd;
    SaveSystem save;
    InputScript script;  // --script (debug)

    vs::SpriteSheet tiles, player, ui;
    vs::Texture logo, menu_bg, panel_bg;
    vs::Font font8, font16, font24, font32;  // Press Start 2P (pixel font)
    vs::Font body, body_small;                 // VT323 32 / 24 px
    vs::Theme theme;

    // Branding art for the intro (smooth, linear filtering). Empty texture if missing.
    vs::Texture branding(const std::string& file);

    // Sound effects by file stem ("step", "door_open", ...). Unknown names play nothing.
    void play(const std::string& name, float volume = 1.0f, float pitch = 1.0f, float pan = 0.0f);
    const vs::Sound* sound(const std::string& name) const;

    // Music by stem ("menu", "game", "game_fast", "victory"); decoded in the background
    // at start-up, waited for on first use.
    const vs::Sound& music(const std::string& name);
    void play_music(const std::string& name, bool loop = true, double crossfade = 1.0);

    // Applies the volume settings to the mixer.
    void apply_volumes();
    // Writes the settings to disk. (settings.fullscreen follows the window through the App's
    // fullscreen listener, installed in main.cpp: F11, Alt+Enter and OPTIONS all persist.)
    void save_settings();

    vs::App& app() { return *app_; }

private:
    vs::App* app_;
    std::map<std::string, vs::Sound> sounds_;
    std::map<std::string, std::shared_future<vs::Sound>> music_;
    std::map<std::string, vs::Sound> music_ready_;
};

using ResourcesPtr = std::shared_ptr<Resources>;

}  // namespace sg
