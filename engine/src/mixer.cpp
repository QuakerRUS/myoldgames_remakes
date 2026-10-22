#include "mixer.hpp"

#include <algorithm>
#include <cmath>

namespace vs::detail {

namespace {
constexpr float kScale = 1.0f / 32768.0f;
}

void Mixer::sample(const SampleBuffer& b, double pos, bool loop, float& l, float& r) {
    const std::size_t frames = b.frames();
    if (frames == 0) return;
    const std::size_t i = static_cast<std::size_t>(pos);
    if (i >= frames) return;
    std::size_t j = i + 1;
    if (j >= frames) j = loop ? 0 : i;
    const float f = static_cast<float>(pos - static_cast<double>(i));
    const std::size_t ch = static_cast<std::size_t>(b.channels);
    const float l0 = b.samples[i * ch] * kScale;
    const float l1 = b.samples[j * ch] * kScale;
    const float left = l0 + (l1 - l0) * f;
    float right = left;
    if (ch >= 2) {
        const float r0 = b.samples[i * ch + 1] * kScale;
        const float r1 = b.samples[j * ch + 1] * kScale;
        right = r0 + (r1 - r0) * f;
    }
    l += left;
    r += right;
}

void Mixer::start_fade(MusicSlot& slot, float target, double seconds, int rate) {
    slot.target = target;
    if (seconds <= 0) {
        slot.gain = target;
        slot.step = 0;
    } else {
        slot.step = static_cast<float>((target - slot.gain) / (seconds * rate));
    }
}

VoiceId Mixer::play(std::shared_ptr<const SampleBuffer> buffer, float volume, float pitch, float pan, bool loop) {
    if (!playable(buffer)) return 0;
    std::lock_guard lock(mutex_);
    if (voices_.size() >= kMaxVoices) voices_.erase(voices_.begin());  // steal the oldest voice
    Voice v;
    v.id = next_id_++;
    if (next_id_ == 0) next_id_ = 1;
    v.buffer = std::move(buffer);
    v.volume = volume;
    v.pitch = pitch > 0.01f ? pitch : 0.01f;
    v.pan = std::clamp(pan, -1.0f, 1.0f);
    v.loop = loop;
    voices_.push_back(std::move(v));
    return voices_.back().id;
}

void Mixer::stop(VoiceId id) {
    std::lock_guard lock(mutex_);
    std::erase_if(voices_, [id](const Voice& v) { return v.id == id; });
}

void Mixer::stop_all() {
    std::lock_guard lock(mutex_);
    voices_.clear();
}

bool Mixer::is_playing(VoiceId id) const {
    std::lock_guard lock(mutex_);
    return std::any_of(voices_.begin(), voices_.end(), [id](const Voice& v) { return v.id == id; });
}

void Mixer::play_music(std::shared_ptr<const SampleBuffer> buffer, bool loop, double crossfade) {
    if (!playable(buffer)) return;
    std::lock_guard lock(mutex_);
    MusicSlot& cur = music_[0];
    if (cur.buffer && cur.buffer == buffer && cur.target > 0) return;
    if (crossfade > 0 && cur.buffer) {
        if (music_[1].buffer) {
            // A crossfade is already running: the louder of the two outgoing tracks fades out
            // over the new crossfade, the quieter one quickly (instead of being cut).
            MusicSlot louder = cur.gain >= music_[1].gain ? cur : music_[1];
            MusicSlot quieter = cur.gain >= music_[1].gain ? music_[1] : cur;
            // Slot 2 may still hold an earlier quick fade (<= kQuickFadeSeconds); keep the louder.
            if (!music_[2].buffer || quieter.gain >= music_[2].gain) {
                music_[2] = quieter;
                start_fade(music_[2], 0.0f, std::min(kQuickFadeSeconds, crossfade), output_rate_);
            }
            music_[1] = louder;
        } else {
            music_[1] = cur;
        }
        start_fade(music_[1], 0.0f, crossfade, output_rate_);
    } else {
        music_[1] = MusicSlot{};
        music_[2] = MusicSlot{};
    }
    cur = MusicSlot{};
    cur.buffer = std::move(buffer);
    cur.loop = loop;
    cur.gain = crossfade > 0 ? 0.0f : 1.0f;
    start_fade(cur, 1.0f, crossfade, output_rate_);
}

void Mixer::stop_music(double fade_out) {
    std::lock_guard lock(mutex_);
    if (fade_out <= 0) {
        for (MusicSlot& m : music_) m = MusicSlot{};
        return;
    }
    start_fade(music_[0], 0.0f, fade_out, output_rate_);
}

bool Mixer::music_playing() const {
    std::lock_guard lock(mutex_);
    return music_[0].buffer != nullptr && music_[0].target > 0;
}

void Mixer::set_music_rate(double rate) {
    std::lock_guard lock(mutex_);
    music_rate_ = std::clamp(rate, 0.1, 4.0);
}

double Mixer::music_rate() const {
    std::lock_guard lock(mutex_);
    return music_rate_;
}

void Mixer::set_volumes(float master, float sfx, float music) {
    std::lock_guard lock(mutex_);
    master_ = std::clamp(master, 0.0f, 1.0f);
    sfx_ = std::clamp(sfx, 0.0f, 1.0f);
    music_volume_ = std::clamp(music, 0.0f, 1.0f);
}

float Mixer::master() const {
    std::lock_guard lock(mutex_);
    return master_;
}
float Mixer::sfx() const {
    std::lock_guard lock(mutex_);
    return sfx_;
}
float Mixer::music() const {
    std::lock_guard lock(mutex_);
    return music_volume_;
}

void Mixer::render(float* out, int frames) {
    std::fill(out, out + static_cast<std::size_t>(frames) * 2, 0.0f);
    std::lock_guard lock(mutex_);
    const double out_rate = output_rate_;

    for (Voice& v : voices_) {
        const SampleBuffer& b = *v.buffer;
        const double step = b.sample_rate / out_rate * v.pitch;
        const double len = static_cast<double>(b.frames());
        const float g = master_ * sfx_ * v.volume;
        const float gl = g * std::min(1.0f, 1.0f - v.pan);
        const float gr = g * std::min(1.0f, 1.0f + v.pan);
        for (int i = 0; i < frames; ++i) {
            if (v.pos >= len) {
                if (!v.loop) break;
                v.pos = std::fmod(v.pos, len);
            }
            float l = 0, r = 0;
            sample(b, v.pos, v.loop, l, r);
            out[2 * i] += l * gl;
            out[2 * i + 1] += r * gr;
            v.pos += step;
        }
    }
    std::erase_if(voices_, [](const Voice& v) { return !v.loop && v.pos >= static_cast<double>(v.buffer->frames()); });

    for (MusicSlot& m : music_) {
        if (!m.buffer) continue;
        const SampleBuffer& b = *m.buffer;
        const double step = b.sample_rate / out_rate * music_rate_;
        const double len = static_cast<double>(b.frames());
        const float g = master_ * music_volume_;
        for (int i = 0; i < frames; ++i) {
            if (m.pos >= len) {
                if (!m.loop) break;
                m.pos = std::fmod(m.pos, len);
            }
            float l = 0, r = 0;
            sample(b, m.pos, m.loop, l, r);
            out[2 * i] += l * g * m.gain;
            out[2 * i + 1] += r * g * m.gain;
            m.pos += step;
            if (m.step != 0) {
                m.gain += m.step;
                if ((m.step > 0 && m.gain >= m.target) || (m.step < 0 && m.gain <= m.target)) {
                    m.gain = m.target;
                    m.step = 0;
                }
            }
        }
        const bool ended = !m.loop && m.pos >= len;
        const bool faded = m.target <= 0 && m.gain <= 0;
        if (ended || faded) m = MusicSlot{};
    }

    for (int i = 0; i < frames * 2; ++i) out[i] = std::clamp(out[i], -1.0f, 1.0f);
}

}  // namespace vs::detail
