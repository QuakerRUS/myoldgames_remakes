#include <kbsim/text_source.hpp>

#include <stdexcept>

#include <kbsim/utf8.hpp>

namespace kbsim {

TextSource::TextSource(std::vector<std::u32string> sentences, std::uint64_t seed) : rng_(seed) {
    for (auto& s : sentences) {
        if (!s.empty()) sentences_.push_back(std::move(s));
    }
    if (sentences_.empty()) throw std::invalid_argument("TextSource: no sentences");
}

TextSource TextSource::load(const std::filesystem::path& file, std::uint64_t seed) {
    std::u32string text;
    try {
        text = utf8_to_u32(read_file(file));
    } catch (const std::invalid_argument& e) {
        throw LoadError(file.string() + ": " + e.what());
    }
    std::vector<std::u32string> sentences;
    std::size_t pos = 0;
    while (pos <= text.size()) {
        std::size_t end = text.find(U'\n', pos);
        if (end == std::u32string::npos) end = text.size();
        std::u32string line = text.substr(pos, end - pos);
        while (!line.empty() && (line.back() == U'\r' || line.back() == U' ' || line.back() == U'\t')) {
            line.pop_back();
        }
        if (!line.empty()) sentences.push_back(std::move(line));
        pos = end + 1;
    }
    if (sentences.empty()) throw LoadError(file.string() + ": no sentences");
    return TextSource(std::move(sentences), seed);
}

const std::u32string& TextSource::next() {
    const std::size_t n = sentences_.size();
    std::size_t index = static_cast<std::size_t>(rng_() % n);
    if (last_ < n) {
        // Same text as the previous one: take the next different text after the drawn index.
        for (std::size_t step = 0; step < n && sentences_[index] == sentences_[last_]; ++step) {
            index = (index + 1) % n;
        }
    }
    last_ = index;
    return sentences_[index];
}

void TextSource::reseed(std::uint64_t seed) {
    rng_.seed(seed);
    last_ = static_cast<std::size_t>(-1);
}

}  // namespace kbsim
