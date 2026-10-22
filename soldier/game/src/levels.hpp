// Access to the shipped levels and the built-in LISIY records.
#pragma once

#include <memory>
#include <optional>
#include <string>

#include <vs/engine/scene.hpp>
#include <soldier/level.hpp>
#include <soldier/recording.hpp>

#include "resources.hpp"

namespace sg {

// data/levels/NN.json; throws (vs::Error / soldier::FormatError) if missing or invalid.
// The returned level's number is always `number` (a different "number" in the file is logged
// and overridden, so progress and records go to the level the file name says).
soldier::Level load_level(int number);
// data/records/LISIY/NN.json, if present and valid.
std::optional<soldier::Recording> builtin_record(int number);

// Scene factories (defined next to each scene) so that scenes can switch to each other
// without including each other's headers.
std::unique_ptr<vs::Scene> make_menu_scene(ResourcesPtr res);
std::unique_ptr<vs::Scene> make_intro_scene(ResourcesPtr res);

}  // namespace sg
