#include "sounds.hpp"

#include <string_view>

#include <vs/engine/storage.hpp>

namespace ks {

namespace {
vs::Sound try_load(std::string_view name) {
    try {
        return vs::Sound::load(vs::asset_path(name));
    } catch (...) {
        return {};
    }
}
}  // namespace

void Sounds::load() {
    correct_ = try_load("sfx/key_correct.wav");
    wrong_ = try_load("sfx/key_wrong.wav");
    sentence_ = try_load("sfx/sentence_complete.wav");
    click_ = try_load("sfx/button_click.wav");
}

void Sounds::play(vs::Audio& audio, Sfx sfx) const {
    if (!enabled || !audio.available()) return;
    const vs::Sound* s = nullptr;
    float volume = 1.0f;
    switch (sfx) {
    case Sfx::Correct: s = &correct_; volume = 0.45f; break;
    case Sfx::Wrong: s = &wrong_; volume = 0.7f; break;
    case Sfx::Sentence: s = &sentence_; volume = 0.75f; break;
    case Sfx::Click: s = &click_; volume = 0.6f; break;
    }
    if (s && s->valid()) audio.play(*s, volume);
}

}  // namespace ks
