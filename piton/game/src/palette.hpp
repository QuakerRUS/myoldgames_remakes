#pragma once
// UI colours of Piton (jungle meadow: cream text, gold highlights, wooden frames).

#include <vs/engine/math.hpp>

namespace piton_game::pal {

inline constexpr vs::Color kText = vs::rgb(0xF4EBD0);
inline constexpr vs::Color kTextDim = vs::rgb(0x9AA58C);
inline constexpr vs::Color kGold = vs::rgb(0xFFD54A);
inline constexpr vs::Color kOrange = vs::rgb(0xFF9A3C);
inline constexpr vs::Color kRed = vs::rgb(0xFF5A4A);
inline constexpr vs::Color kGreen = vs::rgb(0x9CE66A);
inline constexpr vs::Color kWood = vs::rgb(0x8A5A32);
inline constexpr vs::Color kShadow = vs::rgba(0x000000C0);
inline constexpr vs::Color kOutline = vs::rgb(0x14200F);
inline constexpr vs::Color kPanel = vs::rgba(0x142014E6);
inline constexpr vs::Color kDim = vs::rgba(0x000000A0);

}  // namespace piton_game::pal
