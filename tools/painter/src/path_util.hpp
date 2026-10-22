#pragma once
// Small path helpers shared by the Painter core (SDL-free).
#include <filesystem>
#include <string>

namespace painter {

// Lower-case extension of `p` including the dot (".png"), as UTF-8 (vs::path_to_utf8, so a
// non-ANSI file name never throws). Only ASCII letters are lowered; other bytes are kept.
std::string lower_ext(const std::filesystem::path& p);

}  // namespace painter
