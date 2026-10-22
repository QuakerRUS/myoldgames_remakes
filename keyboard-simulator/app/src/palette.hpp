#pragma once
#include <string_view>

#include <kbsim/layout.hpp>
#include <vs/engine/math.hpp>

// Colours of the light theme and of the finger groups.
namespace ks::pal {

inline constexpr vs::Color background = vs::rgb(0xECEFF5);
inline constexpr vs::Color card = vs::rgb(0xFFFFFF);
inline constexpr vs::Color card_alt = vs::rgb(0xF6F8FB);   // keyboard tray, inner panels
inline constexpr vs::Color divider = vs::rgb(0xE3E8F0);
inline constexpr vs::Color ink = vs::rgb(0x1D2433);        // main text
inline constexpr vs::Color ink_soft = vs::rgb(0x3B4456);   // body text in dialogs
inline constexpr vs::Color muted = vs::rgb(0x6B7588);      // captions
inline constexpr vs::Color faint = vs::rgb(0xA6AFBE);      // next sentence, hints
inline constexpr vs::Color accent = vs::rgb(0x3563E9);     // cursor, focus, primary button
inline constexpr vs::Color accent_dark = vs::rgb(0x2448B8);
inline constexpr vs::Color accent_soft = vs::rgb(0xD6E1FD);
inline constexpr vs::Color typed = vs::rgb(0x1F9D68);      // already typed text
inline constexpr vs::Color error = vs::rgb(0xE5484D);
inline constexpr vs::Color error_soft = vs::rgb(0xFDE2E3);
inline constexpr vs::Color key_ink = vs::rgb(0x283041);
inline constexpr vs::Color overlay = vs::rgba(0x141B2C70);

// One finger group: key face, the darker key edge, a saturated accent (legend digits).
struct FingerStyle {
    vs::Color face;
    vs::Color edge;
    vs::Color accent;
};

// The original's colour logic (bg.bmp): green = left pinky + right middle, yellow = ring
// fingers, lavender = left middle + right pinky, pink = left index, cyan = right index,
// grey = thumbs; here in modern pastel tones.
const FingerStyle& finger_style(kbsim::Finger finger);
std::string_view finger_name(kbsim::Finger finger);  // Russian, e.g. "Левый мизинец"

}  // namespace ks::pal
