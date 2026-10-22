#pragma once
#include <functional>
#include <string>
#include <vector>

#include <vs/engine/input.hpp>
#include <vs/engine/renderer.hpp>
#include <vs/engine/texture.hpp>

#include "fonts.hpp"

namespace ks {

// One piece of a dialog's content, laid out top to bottom.
struct Block {
    enum class Kind { Heading, Text, Center, CenterTitle, CenterMuted, Image, Divider, Space };
    Kind kind = Kind::Text;
    std::string text;
    float size = 0;                      // Space: height; Image: side length
    const vs::Texture* image = nullptr;  // Image
};

struct ModalButton {
    std::string label;
    std::function<void()> action;  // called after the dialog has closed
    bool primary = false;
};

// A modal card centred over a dimmed screen (Help, About, the quit question).
// Keys: Enter = the selected button, Esc = the cancel button, Left/Right/Tab move; mouse.
class Modal {
public:
    struct Spec {
        std::string title;
        std::vector<Block> blocks;
        std::vector<ModalButton> buttons;
        float width = 560;
        int cancel = 0;    // button for Esc
        int selected = 0;  // button for Enter
        bool center_buttons = false;
    };

    explicit Modal(const Fonts* fonts, vs::Vec2 screen) : fonts_(fonts), screen_(screen) {}

    void open(Spec spec);
    void close() { open_ = false; }
    bool is_open() const { return open_; }
    std::function<void()> on_click;  // button sound

    void update(const vs::Input& input, double dt);
    void render(vs::Renderer& r) const;

private:
    float block_height(const Block& b) const;
    float content_width() const { return spec_.width - 2 * kPad; }
    void layout();
    void activate(int index);

    static constexpr float kPad = 32.0f;
    static constexpr float kButtonH = 46.0f;

    const Fonts* fonts_;
    vs::Vec2 screen_;
    Spec spec_;
    bool open_ = false;
    float appear_ = 0;  // 0 -> 1 opening animation
    int selected_ = 0;
    int hovered_ = -1;
    vs::RectF card_{};
    std::vector<vs::RectF> button_rects_;
};

}  // namespace ks
