#include <doctest/doctest.h>
#include "mixer.hpp"
#include <vs/engine/error.hpp>

#include <vector>

namespace {
std::shared_ptr<vs::SampleBuffer> constant(std::int16_t value, std::size_t frames, int channels = 1, int rate = 48000) {
    auto b = std::make_shared<vs::SampleBuffer>();
    b->channels = channels;
    b->sample_rate = rate;
    b->samples.assign(frames * static_cast<std::size_t>(channels), value);
    return b;
}
}  // namespace

TEST_CASE("mixer plays a one-shot voice and frees it") {
    vs::detail::Mixer m(48000);
    const vs::VoiceId id = m.play(constant(16384, 10), 1.0f, 1.0f, 0.0f, false);
    CHECK(m.is_playing(id));
    std::vector<float> out(64);
    m.render(out.data(), 32);
    CHECK(out[0] == doctest::Approx(0.5f));
    CHECK(out[1] == doctest::Approx(0.5f));  // mono goes to both channels
    CHECK(out[2 * 20] == 0.0f);              // past the end
    CHECK_FALSE(m.is_playing(id));
}

TEST_CASE("mixer volumes multiply") {
    vs::detail::Mixer m(48000);
    m.set_volumes(0.5f, 0.5f, 1.0f);
    m.play(constant(16384, 100), 1.0f, 1.0f, 0.0f, false);
    std::vector<float> out(8);
    m.render(out.data(), 4);
    CHECK(out[0] == doctest::Approx(0.125f));
}

TEST_CASE("mixer pitch and resampling advance faster") {
    vs::detail::Mixer m(48000);
    // 24 kHz source at pitch 2 -> 1 source frame per output frame -> 10 frames last 10 outputs.
    m.play(constant(8192, 10, 1, 24000), 1.0f, 2.0f, 0.0f, false);
    std::vector<float> out(40);
    m.render(out.data(), 20);
    CHECK(out[2 * 9] != 0.0f);
    CHECK(out[2 * 10] == 0.0f);
}

TEST_CASE("music crossfade replaces the current track") {
    vs::detail::Mixer m(1000);
    auto a = constant(16384, 10000);
    auto b = constant(8192, 10000);
    m.play_music(a, true, 0.0);
    CHECK(m.music_playing());
    std::vector<float> out(2000);
    m.render(out.data(), 10);
    CHECK(out[0] == doctest::Approx(0.5f));
    m.play_music(b, true, 0.5);  // 500 output frames
    m.render(out.data(), 1000);
    CHECK(out[2 * 999] == doctest::Approx(0.25f));  // only b remains, full gain
    m.stop_music(0.0);
    CHECK_FALSE(m.music_playing());
    m.render(out.data(), 4);
    CHECK(out[0] == 0.0f);
}

TEST_CASE("a crossfade during a crossfade does not cut the outgoing tracks") {
    vs::detail::Mixer m(1000);
    auto a = constant(16384, 100000);
    auto b = constant(8192, 100000);
    auto c = constant(4096, 100000);
    std::vector<float> out(4000);
    m.play_music(a, true, 0.0);
    m.render(out.data(), 10);
    m.play_music(b, true, 1.0);  // a: 1 -> 0, b: 0 -> 1 over 1000 frames
    m.render(out.data(), 300);   // a ~0.7, b ~0.3
    const float before = out[2 * 299];
    m.play_music(c, true, 1.0);
    m.render(out.data(), 1);
    // Nothing disappears from one frame to the next: the output stays close to the previous one.
    CHECK(out[0] == doctest::Approx(before).epsilon(0.02));
    m.render(out.data(), 2000);
    CHECK(out[2 * 1999] == doctest::Approx(0.125f));  // only c remains
}

TEST_CASE("buffers with a sample rate <= 0 are rejected") {
    vs::detail::Mixer m(48000);
    CHECK(m.play(constant(16384, 10, 1, 0), 1.0f, 1.0f, 0.0f, false) == 0);
    CHECK(m.play(constant(16384, 10, 1, -44100), 1.0f, 1.0f, 0.0f, false) == 0);
    m.play_music(constant(16384, 10, 1, 0), true, 0.0);
    CHECK_FALSE(m.music_playing());
    CHECK_THROWS_AS(vs::Sound::from_samples(std::vector<std::int16_t>(10, 1), 1, 0), vs::Error);
    CHECK(vs::Sound::from_samples(std::vector<std::int16_t>(10, 1), 1, 22050).valid());
}
