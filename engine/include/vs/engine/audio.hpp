#pragma once
#include <cstdint>
#include <filesystem>
#include <memory>
#include <vector>

namespace vs {

// Decoded PCM audio kept in memory: interleaved int16, 1 or 2 channels, any sample rate.
struct SampleBuffer {
    std::vector<std::int16_t> samples;
    int channels = 2;
    int sample_rate = 48000;
    std::size_t frames() const { return channels > 0 ? samples.size() / static_cast<std::size_t>(channels) : 0; }
};

// A loaded sound or music track. Cheap to copy (shared, immutable data).
class Sound {
public:
    Sound() = default;
    explicit Sound(std::shared_ptr<const SampleBuffer> data) : data_(std::move(data)) {}

    // Loads .wav (SDL_LoadWAV) or .ogg (stb_vorbis) by extension. Throws vs::Error (also for a
    // file that reports a sample rate <= 0).
    static Sound load(const std::filesystem::path& path);
    // Wraps synthesized samples (interleaved int16). Throws vs::Error if sample_rate <= 0.
    static Sound from_samples(std::vector<std::int16_t> samples, int channels, int sample_rate);

    bool valid() const { return data_ && !data_->samples.empty(); }
    double duration() const;  // seconds
    const std::shared_ptr<const SampleBuffer>& data() const { return data_; }

private:
    std::shared_ptr<const SampleBuffer> data_;
};

using VoiceId = std::uint32_t;  // 0 = no voice

// Software mixer on an SDL3 audio stream (48 kHz stereo float output).
// Sound effects play on up to 32 voices; music has its own slots for crossfading.
// Effective volume = master * (sfx | music) * per-call volume.
// If no audio device is available, init() returns false and every call is a silent no-op.
// Created and owned by App (App::audio()); all methods are called from the main thread.
class Audio {
public:
    Audio();
    ~Audio();
    Audio(const Audio&) = delete;
    Audio& operator=(const Audio&) = delete;

    bool init();
    void shutdown();
    bool available() const;

    // Plays a sound effect. pitch is a playback-rate factor (1 = original, 2 = octave up).
    // pan: -1 left .. 0 centre .. 1 right. Returns the voice id (0 if not played, e.g. a
    // buffer with sample_rate <= 0, which is never played).
    VoiceId play(const Sound& sound, float volume = 1.0f, float pitch = 1.0f, float pan = 0.0f, bool loop = false);
    void stop(VoiceId voice);
    void stop_all_sounds();
    bool is_playing(VoiceId voice) const;

    // Starts music. With crossfade > 0 the current track fades out while the new one fades in
    // over that many seconds. Playing the track that is already current does nothing.
    // Starting a crossfade while another is running keeps the louder outgoing track fading
    // and fades the quieter one out quickly (no abrupt cut). A buffer with sample_rate <= 0 is
    // ignored.
    void play_music(const Sound& music, bool loop = true, double crossfade = 0.0);
    void stop_music(double fade_out = 0.0);
    bool music_playing() const;
    // Music playback rate (resampling ratio): 1 = normal, 1.25 = 25% faster (and higher).
    void set_music_rate(double rate);
    double music_rate() const;

    void set_master_volume(float v);
    void set_sfx_volume(float v);
    void set_music_volume(float v);
    float master_volume() const;
    float sfx_volume() const;
    float music_volume() const;

    // Pauses/resumes the whole output (e.g. while the window is minimized).
    void set_paused(bool paused);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace vs
