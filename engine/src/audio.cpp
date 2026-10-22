#include <vs/engine/audio.hpp>

#include <algorithm>
#include <cctype>
#include <string>

#include <SDL3/SDL.h>

#include "mixer.hpp"
#include "stb.hpp"
#include <vs/engine/error.hpp>
#include <vs/engine/json_file.hpp>

namespace vs {

namespace {

constexpr int kOutputRate = 48000;

std::shared_ptr<SampleBuffer> load_wav(const std::vector<unsigned char>& bytes, const std::string& name) {
    SDL_AudioSpec spec{};
    Uint8* data = nullptr;
    Uint32 len = 0;
    SDL_IOStream* io = SDL_IOFromConstMem(bytes.data(), bytes.size());
    if (!io || !SDL_LoadWAV_IO(io, true, &spec, &data, &len)) throw Error("sound: " + name + ": " + SDL_GetError());
    if (spec.freq <= 0) {
        SDL_free(data);
        throw Error("sound: " + name + ": invalid sample rate " + std::to_string(spec.freq));
    }
    const int channels = std::clamp(spec.channels, 1, 2);
    const SDL_AudioSpec dst{SDL_AUDIO_S16, channels, spec.freq};
    Uint8* converted = nullptr;
    int converted_len = 0;
    const bool ok = SDL_ConvertAudioSamples(&spec, data, static_cast<int>(len), &dst, &converted, &converted_len);
    SDL_free(data);
    if (!ok) throw Error("sound: " + name + ": " + SDL_GetError());
    auto buf = std::make_shared<SampleBuffer>();
    buf->channels = channels;
    buf->sample_rate = spec.freq;
    const auto* s = reinterpret_cast<const std::int16_t*>(converted);
    buf->samples.assign(s, s + converted_len / 2);
    SDL_free(converted);
    return buf;
}

std::shared_ptr<SampleBuffer> load_ogg(const std::vector<unsigned char>& bytes, const std::string& name) {
    auto buf = std::make_shared<SampleBuffer>();
    int channels = 0;
    int rate = 0;
    std::vector<std::int16_t> samples;
    if (!stb::decode_vorbis(bytes.data(), bytes.size(), &channels, &rate, samples) || channels <= 0)
        throw Error("sound: cannot decode OGG " + name);
    if (rate <= 0) throw Error("sound: " + name + ": invalid sample rate " + std::to_string(rate));
    if (channels > 2) {  // keep the first two channels
        std::vector<std::int16_t> stereo;
        const std::size_t frames = samples.size() / static_cast<std::size_t>(channels);
        stereo.reserve(frames * 2);
        for (std::size_t i = 0; i < frames; ++i) {
            stereo.push_back(samples[i * static_cast<std::size_t>(channels)]);
            stereo.push_back(samples[i * static_cast<std::size_t>(channels) + 1]);
        }
        samples = std::move(stereo);
        channels = 2;
    }
    buf->channels = channels;
    buf->sample_rate = rate;
    buf->samples = std::move(samples);
    return buf;
}

}  // namespace

Sound Sound::load(const std::filesystem::path& path) {
    std::string ext = path_to_utf8(path.extension());
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    const std::string name = path_to_utf8(path);
    const std::vector<unsigned char> bytes = read_binary_file(path);
    if (ext == ".wav") return Sound(load_wav(bytes, name));
    if (ext == ".ogg") return Sound(load_ogg(bytes, name));
    throw Error("sound: unsupported format " + name);
}

Sound Sound::from_samples(std::vector<std::int16_t> samples, int channels, int sample_rate) {
    if (sample_rate <= 0) throw Error("sound: invalid sample rate " + std::to_string(sample_rate));
    auto buf = std::make_shared<SampleBuffer>();
    buf->samples = std::move(samples);
    buf->channels = std::clamp(channels, 1, 2);
    buf->sample_rate = sample_rate;
    return Sound(std::move(buf));
}

double Sound::duration() const {
    if (!data_ || data_->sample_rate <= 0) return 0.0;
    return static_cast<double>(data_->frames()) / data_->sample_rate;
}

struct Audio::Impl {
    detail::Mixer mixer{kOutputRate};
    SDL_AudioStream* stream = nullptr;
    std::vector<float> scratch;

    static void SDLCALL callback(void* userdata, SDL_AudioStream* stream, int additional, int /*total*/) {
        auto* self = static_cast<Impl*>(userdata);
        const int frames = additional / static_cast<int>(sizeof(float) * 2);
        if (frames <= 0) return;
        self->scratch.resize(static_cast<std::size_t>(frames) * 2);
        self->mixer.render(self->scratch.data(), frames);
        SDL_PutAudioStreamData(stream, self->scratch.data(), frames * static_cast<int>(sizeof(float) * 2));
    }
};

Audio::Audio() : impl_(std::make_unique<Impl>()) {}
Audio::~Audio() { shutdown(); }

bool Audio::init() {
    if (impl_->stream) return true;
    if (!SDL_WasInit(SDL_INIT_AUDIO) && !SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        SDL_Log("audio: %s", SDL_GetError());
        return false;
    }
    const SDL_AudioSpec spec{SDL_AUDIO_F32, 2, kOutputRate};
    impl_->stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, &Impl::callback, impl_.get());
    if (!impl_->stream) {
        SDL_Log("audio: no output device: %s", SDL_GetError());
        return false;
    }
    SDL_ResumeAudioStreamDevice(impl_->stream);
    return true;
}

void Audio::shutdown() {
    if (impl_ && impl_->stream) {
        SDL_DestroyAudioStream(impl_->stream);
        impl_->stream = nullptr;
    }
}

bool Audio::available() const { return impl_->stream != nullptr; }

VoiceId Audio::play(const Sound& sound, float volume, float pitch, float pan, bool loop) {
    if (!available() || !sound.valid()) return 0;
    return impl_->mixer.play(sound.data(), volume, pitch, pan, loop);
}

void Audio::stop(VoiceId voice) { impl_->mixer.stop(voice); }
void Audio::stop_all_sounds() { impl_->mixer.stop_all(); }
bool Audio::is_playing(VoiceId voice) const { return impl_->mixer.is_playing(voice); }

void Audio::play_music(const Sound& music, bool loop, double crossfade) {
    if (!available() || !music.valid()) return;
    impl_->mixer.play_music(music.data(), loop, crossfade);
}

void Audio::stop_music(double fade_out) { impl_->mixer.stop_music(fade_out); }
bool Audio::music_playing() const { return impl_->mixer.music_playing(); }
void Audio::set_music_rate(double rate) { impl_->mixer.set_music_rate(rate); }
double Audio::music_rate() const { return impl_->mixer.music_rate(); }

void Audio::set_master_volume(float v) { impl_->mixer.set_volumes(v, sfx_volume(), music_volume()); }
void Audio::set_sfx_volume(float v) { impl_->mixer.set_volumes(master_volume(), v, music_volume()); }
void Audio::set_music_volume(float v) { impl_->mixer.set_volumes(master_volume(), sfx_volume(), v); }
float Audio::master_volume() const { return impl_->mixer.master(); }
float Audio::sfx_volume() const { return impl_->mixer.sfx(); }
float Audio::music_volume() const { return impl_->mixer.music(); }

void Audio::set_paused(bool paused) {
    if (!impl_->stream) return;
    if (paused)
        SDL_PauseAudioStreamDevice(impl_->stream);
    else
        SDL_ResumeAudioStreamDevice(impl_->stream);
}

}  // namespace vs
