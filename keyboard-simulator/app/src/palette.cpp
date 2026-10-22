#include "palette.hpp"

#include <array>

namespace ks::pal {

namespace {
constexpr FingerStyle kGreen{vs::rgb(0xC9EBD5), vs::rgb(0xA2D5B4), vs::rgb(0x23985E)};
constexpr FingerStyle kYellow{vs::rgb(0xF7EAB4), vs::rgb(0xE2CF83), vs::rgb(0xB88C00)};
constexpr FingerStyle kLavender{vs::rgb(0xDCD8F7), vs::rgb(0xBAB2EA), vs::rgb(0x6555D3)};
constexpr FingerStyle kPink{vs::rgb(0xF7D3E4), vs::rgb(0xE6AAC8), vs::rgb(0xC9407F)};
constexpr FingerStyle kCyan{vs::rgb(0xC8EAF0), vs::rgb(0x9AD2DE), vs::rgb(0x168BA4)};
constexpr FingerStyle kGrey{vs::rgb(0xE4E7EC), vs::rgb(0xC5CAD3), vs::rgb(0x667085)};

constexpr std::array<const FingerStyle*, kbsim::kFingerCount> kStyles = {
    &kGreen, &kYellow, &kLavender, &kPink, &kGrey, &kCyan, &kGreen, &kYellow, &kLavender};

constexpr std::array<std::string_view, kbsim::kFingerCount> kNames = {
    "Левый мизинец",      "Левый безымянный",  "Левый средний",
    "Левый указательный", "Большой палец",     "Правый указательный",
    "Правый средний",     "Правый безымянный", "Правый мизинец"};
}  // namespace

const FingerStyle& finger_style(kbsim::Finger finger) { return *kStyles[static_cast<std::size_t>(finger)]; }

std::string_view finger_name(kbsim::Finger finger) { return kNames[static_cast<std::size_t>(finger)]; }

}  // namespace ks::pal
