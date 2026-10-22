// Sprite-sheet JSON: parse/write through the engine, plus preservation of unknown keys.
#include <set>

#include <vs/engine/error.hpp>

#include "document.hpp"

namespace painter {

namespace {

using ojson = nlohmann::ordered_json;

bool is_engine_key(const std::string& key) {
    return key == "image" || key == "frames" || key == "animations" || key == "grid";
}

}  // namespace

Sheet parse_sheet(std::string_view json_text) {
    Sheet sheet;
    sheet.data = vs::parse_sprite_sheet(json_text);  // validates and expands "grid"
    const ojson root = ojson::parse(json_text);
    for (auto it = root.begin(); it != root.end(); ++it)
        if (!is_engine_key(it.key())) sheet.extra[it.key()] = it.value();
    return sheet;
}

std::string sheet_to_json_text(const Sheet& sheet) {
    const std::string canonical = vs::sprite_sheet_to_json_text(sheet.data);
    if (sheet.extra.empty()) return canonical;
    ojson root = ojson::parse(canonical);
    for (auto it = sheet.extra.begin(); it != sheet.extra.end(); ++it)
        if (!is_engine_key(it.key())) root[it.key()] = it.value();
    return root.dump(2) + "\n";
}

bool sheets_equivalent(const Sheet& a, const Sheet& b, std::string* why) {
    auto fail = [&](std::string msg) {
        if (why) *why = std::move(msg);
        return false;
    };
    const vs::SpriteSheetData& x = a.data;
    const vs::SpriteSheetData& y = b.data;
    if (x.image != y.image) return fail("image \"" + x.image + "\" vs \"" + y.image + "\"");
    if (x.frames.size() != y.frames.size())
        return fail("frame count " + std::to_string(x.frames.size()) + " vs " + std::to_string(y.frames.size()));
    for (std::size_t i = 0; i < x.frames.size(); ++i) {
        const vs::FrameRect& f = x.frames[i];
        const vs::FrameRect& g = y.frames[i];
        if (f.name != g.name || f.x != g.x || f.y != g.y || f.w != g.w || f.h != g.h || f.pivot_x != g.pivot_x ||
            f.pivot_y != g.pivot_y)
            return fail("frame #" + std::to_string(i) + " \"" + f.name + "\" differs");
    }
    if (x.animations.size() != y.animations.size())
        return fail("animation count " + std::to_string(x.animations.size()) + " vs " + std::to_string(y.animations.size()));
    for (std::size_t i = 0; i < x.animations.size(); ++i) {
        const vs::AnimationDef& p = x.animations[i];
        const vs::AnimationDef& q = y.animations[i];
        if (p.name != q.name || p.loop != q.loop || p.frames.size() != q.frames.size())
            return fail("animation \"" + p.name + "\" differs");
        for (std::size_t k = 0; k < p.frames.size(); ++k)
            if (p.frames[k].frame != q.frames[k].frame || p.frames[k].ms != q.frames[k].ms)
                return fail("animation \"" + p.name + "\" entry " + std::to_string(k) + " differs");
    }
    if (a.extra != b.extra) return fail("extra keys differ");
    return true;
}

std::vector<std::string> validate_sheet(const Sheet& sheet, int image_w, int image_h) {
    std::vector<std::string> problems;
    std::set<std::string> names;
    for (const vs::FrameRect& f : sheet.data.frames) {
        if (f.name.empty()) problems.push_back("A frame has an empty name");
        if (!names.insert(f.name).second) problems.push_back("Duplicate frame name \"" + f.name + "\"");
        if (f.w <= 0 || f.h <= 0)
            problems.push_back("Frame \"" + f.name + "\" has an empty size");
        else if (f.x < 0 || f.y < 0 || f.x + f.w > image_w || f.y + f.h > image_h)
            problems.push_back("Frame \"" + f.name + "\" lies outside the image");
    }
    std::set<std::string> anim_names;
    for (const vs::AnimationDef& a : sheet.data.animations) {
        if (a.name.empty()) problems.push_back("An animation has an empty name");
        if (!anim_names.insert(a.name).second) problems.push_back("Duplicate animation name \"" + a.name + "\"");
        if (a.frames.empty()) problems.push_back("Animation \"" + a.name + "\" has no frames");
        for (const vs::AnimationFrame& f : a.frames) {
            if (!names.count(f.frame)) problems.push_back("Animation \"" + a.name + "\" uses unknown frame \"" + f.frame + "\"");
            if (f.ms <= 0) problems.push_back("Animation \"" + a.name + "\" has a non-positive duration");
        }
    }
    return problems;
}

}  // namespace painter
