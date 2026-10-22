#include "save_codec.hpp"

#include <algorithm>
#include <array>
#include <limits>
#include <optional>

#include <piton/types.hpp>

#include "hmac_sha256.hpp"

namespace piton_game {
namespace {

// The signing key, stored XOR-masked so it does not appear as plain text in the executable
// (a deterrent only).
constexpr std::array<unsigned char, 32> kMaskedKey = {
    0x2b, 0x96, 0x4e, 0xd1, 0x07, 0x7a, 0xc3, 0x58, 0x91, 0x3e, 0xe4, 0x6c, 0x15, 0xb2, 0x8f, 0x40,
    0xd9, 0x63, 0x0a, 0xf7, 0x5c, 0x21, 0xae, 0x84, 0x3b, 0xc6, 0x72, 0x19, 0xe8, 0x4d, 0x95, 0x2f};
constexpr std::array<unsigned char, 8> kMask = {0x5a, 0xc3, 0x17, 0x9e, 0x64, 0x2d, 0xb8, 0xf1};

std::string signing_key() {
    std::string key(kMaskedKey.size(), '\0');
    for (std::size_t i = 0; i < kMaskedKey.size(); ++i)
        key[i] = static_cast<char>(kMaskedKey[i] ^ kMask[i % kMask.size()] ^ static_cast<unsigned char>(i * 29u));
    return key;
}

int clamp_volume(int v) { return std::clamp((v + 5) / 10 * 10, 0, 100); }

std::optional<int> get_int(const nlohmann::json& j, const char* key) {
    const auto it = j.find(key);
    if (it == j.end() || !it->is_number_integer()) return std::nullopt;
    const long long v = it->get<long long>();
    if (v < std::numeric_limits<int>::min() || v > std::numeric_limits<int>::max()) return std::nullopt;
    return static_cast<int>(v);
}

}  // namespace

std::string save_signature_message(int level_reached, int high_score) {
    return "piton-save/1\nlevel_reached=" + std::to_string(level_reached) + "\nhigh_score=" + std::to_string(high_score) +
           "\n";
}

std::string save_signature(int level_reached, int high_score) {
    return to_hex(hmac_sha256(signing_key(), save_signature_message(level_reached, high_score)));
}

SaveData save_from_json(const nlohmann::json& j) {
    SaveData s;
    if (!j.is_object()) {
        s.tampered = true;
        return s;
    }
    // Settings: not protected; bad values fall back to the defaults.
    if (const auto v = get_int(j, "sound_volume")) s.sound_volume = clamp_volume(*v);
    if (const auto v = get_int(j, "music_volume")) s.music_volume = clamp_volume(*v);
    if (const auto it = j.find("fullscreen"); it != j.end() && it->is_boolean()) s.fullscreen = it->get<bool>();

    // Progress: only with a valid signature.
    const bool any = j.contains("level_reached") || j.contains("high_score") || j.contains("sig");
    if (!any) return s;  // settings only (nothing to protect)
    const std::optional<int> level = get_int(j, "level_reached");
    const std::optional<int> high = get_int(j, "high_score");
    const auto sig = j.find("sig");
    const bool valid = level && high && sig != j.end() && sig->is_string() &&
                       constant_time_equal(sig->get_ref<const std::string&>(), save_signature(*level, *high));
    if (!valid) {
        s.tampered = true;  // level_reached / high_score keep their defaults (1, 0)
        return s;
    }
    s.level_reached = std::clamp(*level, 1, piton::kLevelCount);
    s.high_score = std::max(0, *high);
    return s;
}

nlohmann::json save_to_json(const SaveData& s) {
    const int level = std::clamp(s.level_reached, 1, piton::kLevelCount);
    const int high = std::max(0, s.high_score);
    nlohmann::json j;
    j["version"] = 1;
    j["level_reached"] = level;
    j["high_score"] = high;
    j["sound_volume"] = clamp_volume(s.sound_volume);
    j["music_volume"] = clamp_volume(s.music_volume);
    j["fullscreen"] = s.fullscreen;
    j["sig"] = save_signature(level, high);
    return j;
}

}  // namespace piton_game
