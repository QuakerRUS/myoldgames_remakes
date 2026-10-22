#pragma once
#include <stdexcept>
#include <string>

namespace vs {
// Thrown by engine functions that load or parse resources (textures, fonts, sounds,
// sprite sheets, JSON). The message names the file and the reason.
class Error : public std::runtime_error {
public:
    explicit Error(const std::string& message) : std::runtime_error(message) {}
};
}  // namespace vs
