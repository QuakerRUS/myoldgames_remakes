#pragma once
// The sentences to type, served in random order.

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <random>
#include <string>
#include <vector>

namespace kbsim {

class TextSource {
public:
    // Takes the sentences as they are; empty strings are dropped. Throws std::invalid_argument
    // if no sentence is left. The same seed gives the same order on every platform.
    TextSource(std::vector<std::u32string> sentences, std::uint64_t seed);

    // Loads a UTF-8 file with one sentence per line (LF or CRLF; empty lines and trailing
    // spaces are skipped). Throws LoadError if the file is missing, not UTF-8 or empty.
    static TextSource load(const std::filesystem::path& file, std::uint64_t seed);

    std::size_t size() const { return sentences_.size(); }
    const std::u32string& sentence(std::size_t index) const { return sentences_.at(index); }
    const std::vector<std::u32string>& sentences() const { return sentences_; }

    // A random sentence. It never equals (by text) the one returned just before, unless the
    // source holds a single distinct text.
    const std::u32string& next();

    // Restarts the random sequence with a new seed (the "previous sentence" is forgotten).
    void reseed(std::uint64_t seed);

private:
    std::vector<std::u32string> sentences_;
    std::mt19937_64 rng_;
    std::size_t last_ = static_cast<std::size_t>(-1);
};

}  // namespace kbsim
