#pragma once
// The edited document (SDL-free): an RGBA image, optionally with sprite-sheet data.
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include <vs/engine/image.hpp>
#include <vs/engine/sprite_sheet_data.hpp>

namespace painter {

// Sprite-sheet data plus the top-level JSON keys the engine does not know (kept on save, so
// nothing written by other tools is lost). "grid" is not kept: its frames are expanded into
// "frames" and written explicitly.
struct Sheet {
    vs::SpriteSheetData data;
    nlohmann::ordered_json extra = nlohmann::ordered_json::object();
};

Sheet parse_sheet(std::string_view json_text);           // throws vs::Error
std::string sheet_to_json_text(const Sheet& sheet);      // the engine's canonical form + extra keys
// Same frames (names, order, rects, pivots), animations (names, order, frames, ms, loop), image
// name and extra keys. On a difference returns false and describes it in `why`.
bool sheets_equivalent(const Sheet& a, const Sheet& b, std::string* why = nullptr);
// Problems that would make SpriteSheet::load fail (frames outside the image, duplicate or
// empty names, empty animations, unknown frames, non-positive durations).
std::vector<std::string> validate_sheet(const Sheet& sheet, int image_w, int image_h);

struct Document {
    vs::Image image;
    std::optional<Sheet> sheet;     // set: a sprite sheet (PNG + JSON)
    std::filesystem::path path;     // the .json of a sheet or the .png of an image; empty = untitled
    int revision = 0;               // bumped on every change of the pixels or the size
    int sheet_revision = 0;         // bumped on every change of the sheet data

    bool is_sheet() const { return sheet.has_value(); }
};

// .json: a sprite sheet (the PNG named by "image", relative to the JSON); anything else: an image.
Document load_document(const std::filesystem::path& path);   // throws vs::Error

// Saves to `target`:
//  - image only: a PNG at target (".png" appended when there is no extension);
//  - sheet + target.json: the PNG next to it (keeps "image" when the stem is unchanged,
//    otherwise "<stem>.png") and the JSON;
//  - sheet + target.png: that PNG and "<stem>.json" next to it.
// Writes are atomic (PNG first, then JSON). Updates doc.path. Throws vs::Error.
void save_document(Document& doc, const std::filesystem::path& target);

// The files save_document(doc, target) writes. `main` is the chosen file (".json"/".png"
// appended when target has no extension), `companion` the other half of a sheet (empty for a
// plain image); `image_name` is the sheet's "image" value after the save.
struct SaveTargets {
    std::filesystem::path main, companion;
    std::filesystem::path json, png;  // json is empty for a plain image
    std::string image_name;
};
SaveTargets save_targets(const Document& doc, const std::filesystem::path& target);

// The companion file save_document(doc, target) would overwrite although it is not part of this
// document (e.g. Save As hero.png while hero.json is another sheet): it exists and is neither
// doc.path nor image_path_of(doc). Empty if there is none. Ask before saving when set.
std::filesystem::path foreign_companion(const Document& doc, const std::filesystem::path& target);

// Path of the PNG a document's pixels live in (for a sheet: next to the JSON).
std::filesystem::path image_path_of(const Document& doc);

}  // namespace painter
