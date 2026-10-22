#pragma once
#include <filesystem>
#include <ostream>

namespace painter {

// Self-check: opens every sprite sheet (*.json) and every other PNG of `in_dir` exactly as
// Painter does, saves it into `out_dir` without edits, reloads both and compares the pixels
// and the JSON semantics (frames, animations, extra keys). Prints one line per file.
// Returns 0 when everything matches, 1 otherwise.
int run_roundtrip(const std::filesystem::path& in_dir, const std::filesystem::path& out_dir, std::ostream& out);

}  // namespace painter
