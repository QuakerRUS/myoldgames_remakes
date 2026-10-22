#pragma once
// SDL-free software mixer behind vs::Audio. Thread-safe: control methods are called from the
// main thread, render() from the audio thread.
#include <memory>
#include <mutex>
#include <vector>

#include <vs/engine/audio.hpp>

namespace vs::detail {

class Mixer {
public:
    explicit Mixer(int output_rate = 48000) : output_rate_(output_rate) {}

    static constexpr std::size_t kMaxVoices = 32;

    VoiceId play(std::shared_ptr<const SampleBuffer> buffer, float volume, float pitch, float pan, bool loop);
    void stop(VoiceId id);
    void stop_all();
    bool is_playing(VoiceId id) const;

    void play_music(std::shared_ptr<const SampleBuffer> buffer, bool loop, double crossfade);
    void stop_music(double fade_out);
    bool music_playing() const;
    void set_music_rate(double rate);
    double music_rate() const;

    void set_volumes(float master, float sfx, float music);
    float master() const;
    float sfx() const;
    float music() const;

    // Mixes `frames` stereo frames (interleaved float, overwritten) at output_rate.
    void render(float* out, int frames);

private:
    struct Voice {
        VoiceId id = 0;
        std::shared_ptr<const SampleBuffer> buffer;
        double pos = 0;
        float volume = 1;
        float pitch = 1;
        float pan = 0;
        bool loop = false;
    };
    struct MusicSlot {
        std::shared_ptr<const SampleBuffer> buffer;
        double pos = 0;
        bool loop = true;
        float gain = 0;
        float target = 0;
        float step = 0;  // gain change per output frame
    };

    // Adds the (linearly interpolated) frame of `buffer` at `pos` to (l, r).
    static void sample(const SampleBuffer& buffer, double pos, bool loop, float& l, float& r);
    static void start_fade(MusicSlot& slot, float target, double seconds, int rate);
    static bool playable(const std::shared_ptr<const SampleBuffer>& buffer) {
        return buffer && buffer->sample_rate > 0 && buffer->channels > 0 && buffer->frames() > 0;
    }

public:
    static constexpr double kQuickFadeSeconds = 0.05;

private:
    mutable std::mutex mutex_;
    int output_rate_;
    std::vector<Voice> voices_;
    VoiceId next_id_ = 1;
    // [0] current, [1] fading out, [2] quick fade-out of a track displaced by a crossfade
    // that started while another crossfade was running.
    MusicSlot music_[3];
    double music_rate_ = 1.0;
    float master_ = 1.0f;
    float sfx_ = 1.0f;
    float music_volume_ = 1.0f;
};

}  // namespace vs::detail
