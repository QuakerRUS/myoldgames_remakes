#include "roundtrip.hpp"

#include <algorithm>
#include <set>
#include <string>
#include <vector>

#include <vs/engine/json_file.hpp>

#include "document.hpp"
#include "path_util.hpp"

namespace painter {

namespace {

namespace fs = std::filesystem;

bool same_pixels(const vs::Image& a, const vs::Image& b, std::string& why) {
    if (a.width != b.width || a.height != b.height) {
        why = "size " + std::to_string(a.width) + "x" + std::to_string(a.height) + " vs " + std::to_string(b.width) + "x" +
              std::to_string(b.height);
        return false;
    }
    if (a.pixels != b.pixels) {
        why = "pixels differ";
        return false;
    }
    return true;
}

std::vector<fs::path> files_with_ext(const fs::path& dir, const char* ext) {
    std::vector<fs::path> out;
    for (const fs::directory_entry& e : fs::directory_iterator(dir))
        if (e.is_regular_file() && lower_ext(e.path()) == ext) out.push_back(e.path());
    std::sort(out.begin(), out.end());
    return out;
}

// Opens `src`, saves it as out_dir/<same name>, compares. Returns true on success.
bool check_one(const fs::path& src, const fs::path& out_dir, std::ostream& out) {
    const std::string name = vs::path_to_utf8(src.filename());
    try {
        Document doc = load_document(src);
        const vs::Image original_pixels = vs::load_image(image_path_of(doc));
        const std::optional<Sheet> original_sheet = doc.sheet;
        const fs::path target = out_dir / src.filename();
        save_document(doc, target);

        std::string why;
        Document saved = load_document(target);
        if (!same_pixels(original_pixels, saved.image, why)) {
            out << "FAIL " << name << ": " << why << "\n";
            return false;
        }
        std::string summary = std::to_string(saved.image.width) + "x" + std::to_string(saved.image.height) + " pixels identical";
        if (original_sheet) {
            if (!saved.sheet || !sheets_equivalent(*original_sheet, *saved.sheet, &why)) {
                out << "FAIL " << name << ": JSON " << (saved.sheet ? why : std::string("lost the sheet")) << "\n";
                return false;
            }
            // The canonical text must be stable: saving the saved file again gives the same bytes.
            if (sheet_to_json_text(*saved.sheet) != vs::read_text_file(target)) {
                out << "FAIL " << name << ": JSON text is not canonical/stable\n";
                return false;
            }
            const std::vector<std::string> problems = validate_sheet(*saved.sheet, saved.image.width, saved.image.height);
            if (!problems.empty()) {
                out << "FAIL " << name << ": " << problems.front() << "\n";
                return false;
            }
            summary += ", " + std::to_string(saved.sheet->data.frames.size()) + " frames, " +
                       std::to_string(saved.sheet->data.animations.size()) + " animations equivalent";
        }
        out << "OK   " << name << ": " << summary << "\n";
        return true;
    } catch (const std::exception& e) {
        out << "FAIL " << name << ": " << e.what() << "\n";
        return false;
    }
}

}  // namespace

int run_roundtrip(const fs::path& in_dir, const fs::path& out_dir, std::ostream& out) {
    std::error_code ec;
    if (!fs::is_directory(in_dir, ec)) {
        out << "FAIL " << vs::path_to_utf8(in_dir) << " is not a directory\n";
        return 1;
    }
    fs::create_directories(out_dir, ec);
    if (fs::equivalent(in_dir, out_dir, ec)) {
        out << "FAIL the output directory must differ from the input directory\n";
        return 1;
    }
    int checked = 0, failed = 0;
    std::set<fs::path> sheet_images;
    for (const fs::path& json : files_with_ext(in_dir, ".json")) {
        ++checked;
        if (!check_one(json, out_dir, out)) ++failed;
        try {
            sheet_images.insert(fs::weakly_canonical(in_dir / vs::utf8_to_path(parse_sheet(vs::read_text_file(json)).data.image)));
        } catch (const std::exception&) {
        }
    }
    for (const fs::path& png : files_with_ext(in_dir, ".png")) {
        if (sheet_images.count(fs::weakly_canonical(png))) continue;  // already checked with its sheet
        ++checked;
        if (!check_one(png, out_dir, out)) ++failed;
    }
    out << (failed ? "FAILED: " : "PASSED: ") << checked - failed << "/" << checked << " files round-trip from "
        << vs::path_to_utf8(in_dir) << " to " << vs::path_to_utf8(out_dir) << "\n";
    return failed ? 1 : 0;
}

}  // namespace painter
