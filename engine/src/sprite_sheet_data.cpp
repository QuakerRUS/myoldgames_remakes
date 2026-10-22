#include <vs/engine/sprite_sheet_data.hpp>

#include <nlohmann/json.hpp>

#include <vs/engine/error.hpp>
#include <vs/engine/json_file.hpp>

namespace vs {

using ojson = nlohmann::ordered_json;

int AnimationDef::total_ms() const {
    int total = 0;
    for (const AnimationFrame& f : frames) total += f.ms;
    return total;
}

const FrameRect* SpriteSheetData::find_frame(std::string_view name) const {
    for (const FrameRect& f : frames)
        if (f.name == name) return &f;
    return nullptr;
}

const AnimationDef* SpriteSheetData::find_animation(std::string_view name) const {
    for (const AnimationDef& a : animations)
        if (a.name == name) return &a;
    return nullptr;
}

namespace {

int get_int(const ojson& obj, const char* key, int fallback) {
    auto it = obj.find(key);
    if (it == obj.end()) return fallback;
    if (!it->is_number_integer()) throw Error(std::string("sprite sheet: \"") + key + "\" must be an integer");
    return it->get<int>();
}

int require_int(const ojson& obj, const char* key, const std::string& where) {
    auto it = obj.find(key);
    if (it == obj.end() || !it->is_number_integer())
        throw Error("sprite sheet: " + where + " needs integer \"" + key + "\"");
    return it->get<int>();
}

FrameRect parse_frame(const std::string& name, const ojson& j) {
    if (!j.is_object()) throw Error("sprite sheet: frame \"" + name + "\" must be an object");
    FrameRect f;
    f.name = name;
    f.x = require_int(j, "x", "frame \"" + name + "\"");
    f.y = require_int(j, "y", "frame \"" + name + "\"");
    f.w = require_int(j, "w", "frame \"" + name + "\"");
    f.h = require_int(j, "h", "frame \"" + name + "\"");
    f.pivot_x = get_int(j, "pivot_x", 0);
    f.pivot_y = get_int(j, "pivot_y", 0);
    if (f.w <= 0 || f.h <= 0) throw Error("sprite sheet: frame \"" + name + "\" has an empty size");
    return f;
}

// "grid": {"frame_w": 32, "frame_h": 32, "columns": 4, "count": 8, "prefix": "f", "margin": 0, "spacing": 0}
void parse_grid(const ojson& g, std::vector<FrameRect>& out) {
    const int fw = require_int(g, "frame_w", "grid");
    const int fh = require_int(g, "frame_h", "grid");
    const int columns = require_int(g, "columns", "grid");
    const int count = require_int(g, "count", "grid");
    const int margin = get_int(g, "margin", 0);
    const int spacing = get_int(g, "spacing", 0);
    const std::string prefix = g.value("prefix", std::string("frame"));
    if (fw <= 0 || fh <= 0 || columns <= 0 || count < 0) throw Error("sprite sheet: invalid grid");
    for (int i = 0; i < count; ++i) {
        FrameRect f;
        f.name = prefix + std::to_string(i);
        f.x = margin + (i % columns) * (fw + spacing);
        f.y = margin + (i / columns) * (fh + spacing);
        f.w = fw;
        f.h = fh;
        f.pivot_x = get_int(g, "pivot_x", 0);
        f.pivot_y = get_int(g, "pivot_y", 0);
        out.push_back(std::move(f));
    }
}

AnimationDef parse_animation(const std::string& name, const ojson& j) {
    if (!j.is_object()) throw Error("sprite sheet: animation \"" + name + "\" must be an object");
    AnimationDef a;
    a.name = name;
    a.loop = j.value("loop", true);
    const int default_ms = get_int(j, "ms", 100);
    auto it = j.find("frames");
    if (it == j.end() || !it->is_array() || it->empty())
        throw Error("sprite sheet: animation \"" + name + "\" needs a non-empty \"frames\" array");
    for (const ojson& e : *it) {
        AnimationFrame f;
        if (e.is_string()) {
            f.frame = e.get<std::string>();
            f.ms = default_ms;
        } else if (e.is_object() && e.contains("frame") && e["frame"].is_string()) {
            f.frame = e["frame"].get<std::string>();
            f.ms = get_int(e, "ms", default_ms);
        } else {
            throw Error("sprite sheet: animation \"" + name + "\" has an invalid frame entry");
        }
        if (f.ms <= 0) throw Error("sprite sheet: animation \"" + name + "\" has a non-positive duration");
        a.frames.push_back(std::move(f));
    }
    return a;
}

}  // namespace

SpriteSheetData parse_sprite_sheet(std::string_view json_text) {
    ojson root;
    try {
        root = ojson::parse(json_text);
    } catch (const nlohmann::json::exception& e) {
        throw Error(std::string("sprite sheet: ") + e.what());
    }
    if (!root.is_object()) throw Error("sprite sheet: the root must be an object");
    SpriteSheetData data;
    auto image = root.find("image");
    if (image == root.end() || !image->is_string()) throw Error("sprite sheet: missing \"image\"");
    data.image = image->get<std::string>();

    if (auto grid = root.find("grid"); grid != root.end()) parse_grid(*grid, data.frames);
    if (auto frames = root.find("frames"); frames != root.end()) {
        if (!frames->is_object()) throw Error("sprite sheet: \"frames\" must be an object");
        for (auto it = frames->begin(); it != frames->end(); ++it) data.frames.push_back(parse_frame(it.key(), it.value()));
    }
    for (std::size_t i = 0; i < data.frames.size(); ++i)
        for (std::size_t k = 0; k < i; ++k)
            if (data.frames[k].name == data.frames[i].name)
                throw Error("sprite sheet: duplicate frame \"" + data.frames[i].name + "\"");

    if (auto anims = root.find("animations"); anims != root.end()) {
        if (!anims->is_object()) throw Error("sprite sheet: \"animations\" must be an object");
        for (auto it = anims->begin(); it != anims->end(); ++it) {
            AnimationDef a = parse_animation(it.key(), it.value());
            for (const AnimationFrame& f : a.frames)
                if (!data.find_frame(f.frame))
                    throw Error("sprite sheet: animation \"" + a.name + "\" uses unknown frame \"" + f.frame + "\"");
            data.animations.push_back(std::move(a));
        }
    }
    return data;
}

SpriteSheetData load_sprite_sheet_data(const std::filesystem::path& json_path) {
    try {
        return parse_sprite_sheet(read_text_file(json_path));
    } catch (const Error& e) {
        throw Error(path_to_utf8(json_path) + ": " + e.what());
    }
}

std::string sprite_sheet_to_json_text(const SpriteSheetData& data) {
    ojson root;
    root["image"] = data.image;
    ojson frames = ojson::object();
    for (const FrameRect& f : data.frames) {
        ojson j;
        j["x"] = f.x;
        j["y"] = f.y;
        j["w"] = f.w;
        j["h"] = f.h;
        if (f.pivot_x != 0 || f.pivot_y != 0) {
            j["pivot_x"] = f.pivot_x;
            j["pivot_y"] = f.pivot_y;
        }
        frames[f.name] = std::move(j);
    }
    root["frames"] = std::move(frames);
    ojson anims = ojson::object();
    for (const AnimationDef& a : data.animations) {
        ojson j;
        j["loop"] = a.loop;
        ojson list = ojson::array();
        for (const AnimationFrame& f : a.frames) list.push_back({{"frame", f.frame}, {"ms", f.ms}});
        j["frames"] = std::move(list);
        anims[a.name] = std::move(j);
    }
    root["animations"] = std::move(anims);
    return root.dump(2) + "\n";
}

}  // namespace vs
