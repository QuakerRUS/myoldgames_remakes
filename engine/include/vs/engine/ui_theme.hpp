#pragma once
#include <functional>
#include <optional>
#include <string>

#include <vs/engine/font.hpp>
#include <vs/engine/math.hpp>

namespace vs {

// UI events a game may want to play a sound for.
enum class UiSound { Move, Select, Cancel, Type, Error };

// Look of the ui_* widgets. Each game fills one Theme and passes a pointer to its widgets;
// the Theme and its fonts must outlive the widgets.
struct Theme {
    const Font* font = nullptr;        // required: labels, items, field text
    const Font* title_font = nullptr;  // optional: dialog message (falls back to font)

    Color text = rgb(0xE8E8E8);
    Color text_disabled = rgb(0x707070);
    Color highlight_text = rgb(0xFFE070);
    Color highlight_bg = rgba(0xFFFFFF22);  // selected menu item background (alpha 0 = none)
    Color panel_bg = rgba(0x101418F0);
    Color panel_border = rgb(0x5A6878);
    Color overlay = rgba(0x00000099);       // dims the screen behind a dialog
    Color button_bg = rgb(0x2C3440);
    Color button_hover = rgb(0x3A4656);
    Color button_pressed = rgb(0x1E242C);
    Color button_text = rgb(0xF0F0F0);
    Color button_border = rgb(0x5A6878);
    Color button_active = rgb(0x3C6EA8);    // Button::active (toggled) background
    Color field_bg = rgb(0x0C0E12);
    Color field_border = rgb(0x5A6878);
    Color caret = rgb(0xFFE070);

    float corner_radius = 8.0f;
    float border_width = 2.0f;
    float padding = 12.0f;
    float item_spacing = 8.0f;           // gap between menu items
    std::optional<Color> text_shadow;    // applied to menu items and dialog text
    std::optional<Color> text_outline;
    std::string menu_marker;             // optional prefix drawn before the selected item (e.g. "> ")

    // Called by widgets on navigation/selection (play a click sound here). May be empty.
    std::function<void(UiSound)> on_sound;

    void sound(UiSound s) const {
        if (on_sound) on_sound(s);
    }
    TextStyle style(Color color, Align align = Align::Left) const {
        TextStyle st;
        st.color = color;
        st.align = align;
        st.shadow = text_shadow;
        st.outline = text_outline;
        return st;
    }
};

}  // namespace vs
