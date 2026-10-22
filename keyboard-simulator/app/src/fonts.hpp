#pragma once
#include <vs/engine/font.hpp>
#include <vs/engine/renderer.hpp>

namespace ks {

// Every font size the app uses (PT Sans for the UI, PT Mono for the typing text).
struct Fonts {
    vs::Font ui;          // buttons, body text
    vs::Font ui_bold;     // stat values, headings
    vs::Font small;       // captions
    vs::Font small_bold;  // hotkey chips, legend digits
    vs::Font mono;        // the sentences
    vs::Font key;         // letter keys
    vs::Font key_small;   // shifted symbols, modifier captions
    vs::Font title;       // dialog titles
    vs::Font big;         // the "next key" preview

    static Fonts load(vs::Renderer& r);
};

}  // namespace ks
