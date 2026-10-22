#pragma once
// SHA-256 (FIPS 180-4) and HMAC-SHA256 (RFC 2104), self-contained and SDL-free. Used to sign
// the protected fields of piton.json (save_codec.hpp).

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

namespace piton_game {

using Sha256Digest = std::array<std::uint8_t, 32>;

Sha256Digest sha256(std::string_view data);
Sha256Digest hmac_sha256(std::string_view key, std::string_view message);

// Lower-case hex, two characters per byte.
std::string to_hex(const Sha256Digest& digest);

// Compares two strings in time independent of where they differ.
bool constant_time_equal(std::string_view a, std::string_view b);

}  // namespace piton_game
