#pragma once
#include <array>
#include <functional>

#include <kbsim/session.hpp>
#include <vs/engine/input.hpp>
#include <vs/engine/renderer.hpp>
#include <vs/engine/ui_button.hpp>
#include <vs/engine/ui_theme.hpp>

#include "fonts.hpp"

namespace ks {

// The right column: Рестарт / Пауза / Помощь / О программе, the "next key" card and the
// sound switch. Clicks and hotkeys come from vs::Button; the look is drawn here.
class SidePanel {
public:
    struct Actions {
        std::function<void()> restart, pause, help, about, sound;
    };

    // `buttons_area`: the four buttons; `card_area`: the next-key card; `sound_area`: the switch.
    SidePanel(const vs::Theme* theme, vs::RectF buttons_area, vs::RectF card_area, vs::RectF sound_area,
              Actions actions);

    void update(const vs::Input& input);
    void render(vs::Renderer& r, const Fonts& f, const kbsim::Session& s, bool user_paused, bool sound_on,
                double time_s) const;

private:
    void draw_button(vs::Renderer& r, const Fonts& f, const vs::Button& b, std::string_view label,
                     std::string_view hotkey, bool primary) const;
    void draw_next_card(vs::Renderer& r, const Fonts& f, const kbsim::Session& s) const;
    void draw_sound(vs::Renderer& r, const Fonts& f, bool on) const;

    std::array<vs::Button, 4> buttons_;
    vs::Button sound_;
    vs::RectF card_;
};

}  // namespace ks
