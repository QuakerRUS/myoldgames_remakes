#pragma once
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace vs {

// One named frame: a rectangle in the sheet image plus a pivot (relative to the frame's
// top-left corner). Drawing a frame at a position puts the pivot on that position.
struct FrameRect {
    std::string name;
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;
    int pivot_x = 0;
    int pivot_y = 0;
};

struct AnimationFrame {
    std::string frame;  // frame name
    int ms = 100;       // how long the frame is shown, milliseconds (> 0)
};

struct AnimationDef {
    std::string name;
    std::vector<AnimationFrame> frames;
    bool loop = true;
    int total_ms() const;
};

// Contents of a sprite sheet JSON file (format: engine/README.md, "Sprite sheet JSON").
// Frames and animations keep the order of the file.
struct SpriteSheetData {
    std::string image;  // image file name, relative to the JSON file
    std::vector<FrameRect> frames;
    std::vector<AnimationDef> animations;

    const FrameRect* find_frame(std::string_view name) const;
    const AnimationDef* find_animation(std::string_view name) const;
};

// Parses sprite sheet JSON text. Throws vs::Error on malformed input, duplicate names or
// when an animation refers to an unknown frame.
SpriteSheetData parse_sprite_sheet(std::string_view json_text);
// Reads and parses a sprite sheet JSON file. Throws vs::Error.
SpriteSheetData load_sprite_sheet_data(const std::filesystem::path& json_path);
// Serializes to JSON text in the canonical form (frames as objects, animations with
// per-frame "ms"). parse_sprite_sheet(sprite_sheet_to_json_text(d)) reproduces d.
std::string sprite_sheet_to_json_text(const SpriteSheetData& data);

}  // namespace vs
