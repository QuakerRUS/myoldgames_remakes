#include "resources.hpp"

#include <SDL3/SDL_log.h>
#include <vs/engine/error.hpp>
#include <vs/engine/storage.hpp>

namespace sg {

namespace {

const char* const kSoundNames[] = {
    "box_sink",   "button_press", "button_release", "death",      "door_close", "door_open",
    "door_unlock", "drown",       "error",          "give_up",    "guillotine_chop", "key_drop",
    "key_pickup", "level_win",    "menu_move",      "menu_select", "push_box",  "step",
    "step_2",     "timer_tick",   "turn",           "wall_reveal",
};
const char* const kMusicNames[] = {"menu", "game", "game_fast", "victory"};

vs::Font load_pixel_font(vs::Renderer& r, float size) {
    vs::FontOptions o;
    o.oversample = 1;
    o.smooth = false;
    return vs::Font::load(r, vs::asset_path("fonts/PressStart2P-Regular.ttf"), size, o);
}

}  // namespace

Resources::Resources(vs::App& app, CommandLine command_line)
    : cmd(std::move(command_line)), app_(&app) {
    try {
        save.load();  // does not throw; belt and braces: a bad save must never stop the game
    } catch (const std::exception& e) {
        SDL_Log("save: %s", e.what());
    }
    script.parse(cmd.script);
    vs::Renderer& r = app.renderer();

    // Music decoding is the slowest part of the start-up: do it in the background.
    for (const char* m : kMusicNames) {
        const std::filesystem::path p = vs::find_asset(std::string("music/") + m + ".ogg");
        if (p.empty()) continue;
        music_[m] = std::async(std::launch::async, [p] {
                        try {
                            return vs::Sound::load(p);
                        } catch (const std::exception& e) {
                            SDL_Log("music: %s", e.what());
                            return vs::Sound{};
                        }
                    }).share();
    }

    tiles = vs::SpriteSheet::load(r, vs::asset_path("gfx/tiles.json"));
    player = vs::SpriteSheet::load(r, vs::asset_path("gfx/player.json"));
    ui = vs::SpriteSheet::load(r, vs::asset_path("gfx/ui.json"));
    logo = vs::Texture::load(r, vs::asset_path("gfx/logo.png"), vs::ScaleMode::Linear);
    menu_bg = vs::Texture::load(r, vs::asset_path("gfx/menu_bg.png"));
    panel_bg = vs::Texture::load(r, vs::asset_path("gfx/panel_bg.png"));

    font8 = load_pixel_font(r, 8);
    font16 = load_pixel_font(r, 16);
    font24 = load_pixel_font(r, 24);
    font32 = load_pixel_font(r, 32);
    vs::FontOptions crisp;
    crisp.oversample = 1;
    crisp.smooth = false;
    body = vs::Font::load(r, vs::asset_path("fonts/VT323-Regular.ttf"), 32, crisp);
    body_small = vs::Font::load(r, vs::asset_path("fonts/VT323-Regular.ttf"), 24, crisp);

    for (const char* s : kSoundNames) {
        const std::filesystem::path p = vs::find_asset(std::string("sfx/") + s + ".wav");
        if (p.empty()) continue;
        try {
            sounds_[s] = vs::Sound::load(p);
        } catch (const std::exception& e) {
            SDL_Log("sound: %s", e.what());
        }
    }

    theme.font = &font16;
    theme.title_font = &font16;
    theme.text = colors::text;
    theme.highlight_text = colors::accent;
    theme.caret = colors::accent;
    theme.on_sound = [this](vs::UiSound s) {
        switch (s) {
        case vs::UiSound::Move: play("menu_move", 0.6f); break;
        case vs::UiSound::Select: play("menu_select", 0.7f); break;
        case vs::UiSound::Cancel: play("menu_move", 0.5f, 0.8f); break;
        case vs::UiSound::Type: play("menu_move", 0.35f, 1.4f); break;
        case vs::UiSound::Error: play("error", 0.7f); break;
        }
    };

    apply_volumes();
}

vs::Texture Resources::branding(const std::string& file) {
    std::filesystem::path p = vs::find_asset("branding/" + file);
#ifdef VS_BRANDING_DIR
    if (p.empty()) {
        const std::filesystem::path dev = vs::utf8_to_path(VS_BRANDING_DIR) / file;
        std::error_code ec;
        if (std::filesystem::is_regular_file(dev, ec)) p = dev;
    }
#endif
    if (p.empty()) {
        SDL_Log("branding image missing: %s", file.c_str());
        return {};
    }
    return vs::Texture::load(app_->renderer(), p, vs::ScaleMode::Linear);
}

const vs::Sound* Resources::sound(const std::string& name) const {
    const auto it = sounds_.find(name);
    return it == sounds_.end() ? nullptr : &it->second;
}

void Resources::play(const std::string& name, float volume, float pitch, float pan) {
    if (const vs::Sound* s = sound(name)) app_->audio().play(*s, volume, pitch, pan);
}

const vs::Sound& Resources::music(const std::string& name) {
    static const vs::Sound none;
    if (const auto it = music_ready_.find(name); it != music_ready_.end()) return it->second;
    const auto it = music_.find(name);
    if (it == music_.end()) return none;
    music_ready_[name] = it->second.get();
    return music_ready_[name];
}

void Resources::play_music(const std::string& name, bool loop, double crossfade) {
    const vs::Sound& m = music(name);
    if (m.valid()) app_->audio().play_music(m, loop, crossfade);
}

void Resources::apply_volumes() {
    vs::Audio& a = app_->audio();
    a.set_sfx_volume(static_cast<float>(save.settings.sound) / 10.0f);
    a.set_music_volume(static_cast<float>(save.settings.music) / 10.0f);
}

void Resources::save_settings() { save.save(); }

}  // namespace sg
