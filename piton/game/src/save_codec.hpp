#pragma once
// piton.json <-> SaveData, SDL-free (unit-tested in piton/game/tests).
//
// The progress fields (level_reached, high_score) are signed: "sig" is the lower-case hex
// HMAC-SHA256 of save_signature_message() under a key compiled into the program. Editing the
// progress by hand (or removing "sig") is detected on load: the progress is reset to level 1
// and high score 0, the settings (volumes, fullscreen) are kept, and SaveData::tampered is set
// so the game can tell the player and write a correctly signed file. This is a deterrent, not
// real security: the key is in the executable.

#include <string>

#include <nlohmann/json.hpp>

#include "save_data.hpp"

namespace piton_game {

// The canonical bytes that are signed, e.g. "piton-save/1\nlevel_reached=3\nhigh_score=1200\n".
std::string save_signature_message(int level_reached, int high_score);
// Hex HMAC-SHA256 of save_signature_message() with the built-in key.
std::string save_signature(int level_reached, int high_score);

// Never throws. A JSON value that is not an object, or progress fields that are not integers,
// or a missing / wrong "sig" mark the data as tampered (progress reset, settings kept). A file
// with no progress fields and no "sig" at all (settings only) is not tampered.
SaveData save_from_json(const nlohmann::json& j);
// Includes "sig". The values are clamped to their valid ranges first.
nlohmann::json save_to_json(const SaveData& s);

}  // namespace piton_game
