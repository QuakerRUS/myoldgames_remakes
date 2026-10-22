#include "fonts.hpp"

#include <vs/engine/storage.hpp>

namespace ks {

Fonts Fonts::load(vs::Renderer& r) {
    const auto sans = vs::asset_path("fonts/PTSans-Regular.ttf");
    const auto bold = vs::asset_path("fonts/PTSans-Bold.ttf");
    const auto mono = vs::asset_path("fonts/PTMono-Regular.ttf");
    Fonts f;
    f.ui = vs::Font::load(r, sans, 19);
    f.ui_bold = vs::Font::load(r, bold, 19);
    f.small = vs::Font::load(r, sans, 15);
    f.small_bold = vs::Font::load(r, bold, 14);
    f.mono = vs::Font::load(r, mono, 27);
    f.key = vs::Font::load(r, bold, 21);
    f.key_small = vs::Font::load(r, bold, 14);
    f.title = vs::Font::load(r, bold, 28);
    f.big = vs::Font::load(r, bold, 58);
    return f;
}

}  // namespace ks
