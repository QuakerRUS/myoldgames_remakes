#pragma once
#include <vs/engine/audio.hpp>

namespace ks {

enum class Sfx { Correct, Wrong, Sentence, Click };

// The four sound effects; play() does nothing while muted or without an audio device.
class Sounds {
public:
    void load();  // missing files leave that effect silent
    void play(vs::Audio& audio, Sfx sfx) const;
    bool enabled = true;

private:
    vs::Sound correct_, wrong_, sentence_, click_;
};

}  // namespace ks
