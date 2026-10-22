#pragma once
// Colour palettes (SDL-free): built from an image, sorted, and stored as .hex or .gpl files.
#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <vs/engine/image.hpp>
#include <vs/engine/math.hpp>

namespace painter {

using Palette = std::vector<vs::Color>;

// The distinct colours of an image, sorted (transparent first, then greys by lightness, then
// by hue and lightness). Fully transparent pixels count as one entry {0,0,0,0}. With more than
// `max_colors` colours the most frequent ones are kept.
Palette palette_from_image(const vs::Image& image, std::size_t max_colors = 256);
void sort_palette(Palette& palette);

// "RRGGBB" or, with alpha < 255 (or force_alpha), "RRGGBBAA"; upper-case, no '#'.
std::string to_hex(vs::Color c, bool force_alpha = false);
// Accepts "RRGGBB", "RRGGBBAA", optionally with a leading '#'; nullopt otherwise.
std::optional<vs::Color> parse_hex(std::string_view text);

// .hex: one colour per line (Lospec format, 8 digits when alpha < 255).
// .gpl: GIMP palette ("GIMP Palette", "Name:", "Columns:", "R G B name" lines; no alpha).
// Both throw vs::Error on unreadable or malformed files.
Palette parse_hex_palette(std::string_view text);
Palette parse_gpl_palette(std::string_view text);
std::string hex_palette_text(const Palette& palette);
std::string gpl_palette_text(const Palette& palette, std::string_view name);
Palette load_palette(const std::filesystem::path& path);  // by extension (.gpl, else .hex)
void save_palette(const std::filesystem::path& path, const Palette& palette);

}  // namespace painter
