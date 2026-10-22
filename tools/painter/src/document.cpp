#include "document.hpp"

#include "path_util.hpp"

#include <vs/engine/error.hpp>
#include <vs/engine/json_file.hpp>

namespace painter {

std::string lower_ext(const std::filesystem::path& p) {
    std::string e = vs::path_to_utf8(p.extension());
    for (char& ch : e)
        if (ch >= 'A' && ch <= 'Z') ch = static_cast<char>(ch - 'A' + 'a');
    return e;
}

std::filesystem::path image_path_of(const Document& doc) {
    if (doc.sheet && !doc.path.empty()) return doc.path.parent_path() / vs::utf8_to_path(doc.sheet->data.image);
    return doc.path;
}

Document load_document(const std::filesystem::path& path) {
    Document doc;
    if (lower_ext(path) == ".json") {
        Sheet sheet;
        try {
            sheet = parse_sheet(vs::read_text_file(path));
        } catch (const std::exception& e) {
            throw vs::Error(vs::path_to_utf8(path) + ": " + e.what());
        }
        doc.image = vs::load_image(path.parent_path() / vs::utf8_to_path(sheet.data.image));
        doc.sheet = std::move(sheet);
    } else {
        doc.image = vs::load_image(path);
    }
    doc.path = path;
    return doc;
}

SaveTargets save_targets(const Document& doc, const std::filesystem::path& target) {
    SaveTargets out;
    std::filesystem::path t = target;
    if (!t.has_extension()) t += doc.sheet ? ".json" : ".png";
    if (!doc.sheet) {
        out.png = t;
        out.main = t;
        return out;
    }
    out.image_name = doc.sheet->data.image;
    if (lower_ext(t) == ".json") {
        const bool same_stem = !doc.path.empty() && doc.path.stem() == t.stem() && !out.image_name.empty();
        if (!same_stem) out.image_name = vs::path_to_utf8(t.stem()) + ".png";
        out.json = t;
        out.png = t.parent_path() / vs::utf8_to_path(out.image_name);
        out.main = out.json;
        out.companion = out.png;
    } else {
        out.image_name = vs::path_to_utf8(t.filename());
        out.png = t;
        out.json = t;
        out.json.replace_extension(".json");
        out.main = out.png;
        out.companion = out.json;
    }
    return out;
}

namespace {

bool same_file(const std::filesystem::path& a, const std::filesystem::path& b) {
    std::error_code ec;
    if (std::filesystem::equivalent(a, b, ec)) return true;
    return std::filesystem::absolute(a, ec).lexically_normal() == std::filesystem::absolute(b, ec).lexically_normal();
}

}  // namespace

std::filesystem::path foreign_companion(const Document& doc, const std::filesystem::path& target) {
    const SaveTargets t = save_targets(doc, target);
    if (t.companion.empty()) return {};
    std::error_code ec;
    if (!std::filesystem::exists(t.companion, ec)) return {};
    if (!doc.path.empty()) {
        if (same_file(t.companion, doc.path)) return {};
        if (doc.sheet && same_file(t.companion, image_path_of(doc))) return {};
    }
    return t.companion;
}

void save_document(Document& doc, const std::filesystem::path& target) {
    if (doc.image.empty()) throw vs::Error("cannot save an empty image");
    const SaveTargets t = save_targets(doc, target);

    if (!doc.sheet) {
        if (lower_ext(t.png) == ".json") throw vs::Error("a plain image is saved as PNG; use Sheet > Make sprite sheet first");
        vs::save_png(t.png, doc.image);
        doc.path = t.png;
        return;
    }

    Sheet sheet = *doc.sheet;
    sheet.data.image = t.image_name;
    vs::save_png(t.png, doc.image);
    vs::write_file_atomic(t.json, sheet_to_json_text(sheet));
    if (sheet.data.image != doc.sheet->data.image) {
        doc.sheet = std::move(sheet);
        ++doc.sheet_revision;
    }
    doc.path = t.json;
}

}  // namespace painter
