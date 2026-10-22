#include "assets.hpp"

#include <vs/engine/storage.hpp>

#include "palette.hpp"

namespace piton_game {
namespace {

vs::Font pixel_font(vs::Renderer& r, const char* file, float size) {
    return vs::Font::load(r, vs::asset_path(file), size, {.oversample = 1.0f, .smooth = false});
}

}  // namespace

void Assets::load(vs::Renderer& r) {
    const char* ps2p = "fonts/PressStart2P-Regular.ttf";
    font8 = pixel_font(r, ps2p, 8);
    font16 = pixel_font(r, ps2p, 16);
    font24 = pixel_font(r, ps2p, 24);
    font32 = pixel_font(r, ps2p, 32);
    font48 = pixel_font(r, ps2p, 48);
    vt32 = pixel_font(r, "fonts/VT323-Regular.ttf", 32);

    tiles = vs::SpriteSheet::load(r, vs::asset_path("gfx/tiles.json"));
    snake = vs::SpriteSheet::load(r, vs::asset_path("gfx/snake.json"));
    rabbit = vs::SpriteSheet::load(r, vs::asset_path("gfx/rabbit.json"));
    ui = vs::SpriteSheet::load(r, vs::asset_path("gfx/ui.json"));
    logo = vs::Texture::load(r, vs::asset_path("gfx/logo.png"));
    menu_bg = vs::Texture::load(r, vs::asset_path("gfx/menu_bg.png"));

    eat = vs::Sound::load(vs::asset_path("sfx/eat.wav"));
    turn = vs::Sound::load(vs::asset_path("sfx/turn.wav"));
    die = vs::Sound::load(vs::asset_path("sfx/die.wav"));
    level_complete = vs::Sound::load(vs::asset_path("sfx/level_complete.wav"));
    menu_move = vs::Sound::load(vs::asset_path("sfx/menu_move.wav"));
    menu_select = vs::Sound::load(vs::asset_path("sfx/menu_select.wav"));
    music_menu = vs::Sound::load(vs::asset_path("music/menu.ogg"));
    music_game = vs::Sound::load(vs::asset_path("music/game.ogg"));
    music_victory = vs::Sound::load(vs::asset_path("music/victory.ogg"));
}

Shared::Shared(vs::App& app) : app_(&app) {
    assets.load(app.renderer());
    save = SaveData::load();
    if (save.tampered) {
        progress_reset_notice = true;
        save.tampered = false;
        save.save();  // a correctly signed file (progress reset, settings kept)
    }
    apply_volumes();
    app.set_fullscreen_listener([this](bool on) {
        if (save.fullscreen == on) return;
        save.fullscreen = on;
        save.save();
    });

    theme.font = &assets.font24;
    theme.title_font = &assets.font16;
    theme.text = pal::kText;
    theme.text_disabled = pal::kTextDim;
    theme.highlight_text = pal::kGold;
    theme.highlight_bg = vs::rgba(0xFFE07018);
    theme.panel_bg = vs::rgba(0x1B2A1CF2);
    theme.panel_border = pal::kWood;
    theme.overlay = vs::rgba(0x00000088);
    theme.button_bg = vs::rgb(0x2E4A2A);
    theme.button_hover = vs::rgb(0x3E6A38);
    theme.button_pressed = vs::rgb(0x223820);
    theme.button_text = pal::kText;
    theme.button_border = pal::kWood;
    theme.button_active = vs::rgb(0x5C8A3A);
    theme.corner_radius = 4;
    theme.border_width = 3;
    theme.padding = 14;
    theme.item_spacing = 18;
    theme.text_shadow = vs::rgba(0x000000C0);
    theme.on_sound = [this](vs::UiSound s) {
        if (s == vs::UiSound::Move) play(Sfx::MenuMove, 0.7f);
        if (s == vs::UiSound::Select) play(Sfx::MenuSelect);
        if (s == vs::UiSound::Cancel) play(Sfx::MenuMove, 0.7f, 0.8f);
    };
}

Shared::~Shared() { app_->set_fullscreen_listener({}); }

void Shared::play(Sfx s, float volume, float pitch) {
    if (muted) return;
    const vs::Sound* snd = nullptr;
    switch (s) {
    case Sfx::Eat: snd = &assets.eat; break;
    case Sfx::Turn: snd = &assets.turn; break;
    case Sfx::Die: snd = &assets.die; break;
    case Sfx::LevelComplete: snd = &assets.level_complete; break;
    case Sfx::MenuMove: snd = &assets.menu_move; break;
    case Sfx::MenuSelect: snd = &assets.menu_select; break;
    }
    if (snd) app_->audio().play(*snd, volume, pitch);
}

void Shared::music(Music m, bool loop) {
    const vs::Sound& track = m == Music::Menu ? assets.music_menu : m == Music::Game ? assets.music_game : assets.music_victory;
    app_->audio().play_music(track, loop, 0.4);
}

void Shared::stop_music(double fade) { app_->audio().stop_music(fade); }

void Shared::apply_volumes() {
    app_->audio().set_sfx_volume(static_cast<float>(save.sound_volume) / 100.0f);
    app_->audio().set_music_volume(static_cast<float>(save.music_volume) / 100.0f);
}

std::shared_ptr<Shared> Shared::ensure(std::shared_ptr<Shared> existing, vs::App& app) {
    if (existing) return existing;
    return std::make_shared<Shared>(app);
}

}  // namespace piton_game
