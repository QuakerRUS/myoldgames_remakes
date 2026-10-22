#include <vs/editor/run_tile_editor.hpp>
#include <vs/engine/error.hpp>
#include <vs/engine/json_file.hpp>

#include "demo.hpp"

namespace demo {

namespace {

// Minimal plugin: floor/wall/exit/player; exactly one player (placing moves it).
class DemoPlugin : public vs::TileEditorPlugin {
public:
    std::string title() const override { return "Demo Map Editor"; }
    std::string game_id() const override { return "Engine Demo"; }
    std::vector<vs::TileInfo> palette() const override {
        return {{0, "floor", "", vs::rgb(0x303840)},
                {1, "wall", "", vs::rgb(0x8A7A60)},
                {2, "exit", "", vs::rgb(0x40C060)},
                {3, "player", "", vs::rgb(0x4080FF)}};
    }
    std::vector<vs::CellChange> place(const vs::TileDocument& doc, int x, int y, int code) const override {
        std::vector<vs::CellChange> changes;
        if (code == 3)
            for (int yy = 0; yy < doc.grid.height(); ++yy)
                for (int xx = 0; xx < doc.grid.width(); ++xx)
                    if (doc.grid.at(xx, yy) == 3) changes.push_back({xx, yy, 0});
        changes.push_back({x, y, code});
        return changes;
    }
    std::vector<std::string> validate(const vs::TileDocument& doc) const override {
        std::vector<std::string> p;
        if (doc.grid.count(3) != 1) p.push_back("Exactly one player is required");
        if (doc.grid.count(2) == 0) p.push_back("No exit");
        return p;
    }
    bool properties_ui(vs::TileDocument& doc) override {
        int size[2] = {doc.grid.width(), doc.grid.height()};
        if (ImGui::InputInt2("Size", size) && size[0] > 0 && size[1] > 0 && size[0] <= 100 && size[1] <= 100) {
            doc.grid.resize(size[0], size[1]);
            return true;
        }
        return false;
    }
    vs::TileDocument new_document() const override { return {vs::TileGrid(20, 12, 0), {}}; }
    vs::TileDocument load(const std::filesystem::path& path) const override {
        const auto j = vs::read_json_file(path);
        if (!j) throw vs::Error("file not found");
        vs::TileDocument d{vs::TileGrid(j->at("width").get<int>(), j->at("height").get<int>()), {}};
        const auto cells = j->at("cells").get<std::vector<int>>();
        for (int i = 0; i < static_cast<int>(cells.size()) && i < d.grid.width() * d.grid.height(); ++i)
            d.grid.set(i % d.grid.width(), i / d.grid.width(), cells[static_cast<std::size_t>(i)]);
        return d;
    }
    void save(const std::filesystem::path& path, const vs::TileDocument& doc) const override {
        vs::write_json_file(path, {{"width", doc.grid.width()}, {"height", doc.grid.height()}, {"cells", doc.grid.cells()}});
    }
};

}  // namespace

int run_demo_editor(int argc, char** argv) {
    DemoPlugin plugin;
    std::vector<char*> args;
    for (int i = 0; i < argc; ++i)
        if (std::string(argv[i]) != "--editor") args.push_back(argv[i]);
    return vs::run_tile_editor(plugin, static_cast<int>(args.size()), args.data());
}

}  // namespace demo
