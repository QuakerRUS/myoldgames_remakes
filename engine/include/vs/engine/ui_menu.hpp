#pragma once
#include <functional>
#include <string>
#include <vector>

#include <vs/engine/input.hpp>
#include <vs/engine/renderer.hpp>
#include <vs/engine/ui_theme.hpp>

namespace vs {

struct MenuItem {
    std::string label;
    std::function<void()> on_select;          // Enter / Space / click
    std::function<void(int dir)> on_adjust;   // Left (-1) / Right (+1), e.g. "GAME SPEED  x1"
    bool enabled = true;
};

// Vertical list menu. Keyboard: Up/Down (with key repeat, wrapping, skipping disabled items),
// Enter/Space select, Left/Right adjust, Esc calls on_cancel. Mouse: hovering selects,
// left click activates, wheel moves the selection.
class Menu {
public:
    explicit Menu(const Theme* theme) : theme_(theme) {}

    // Adds an item and returns its index.
    int add(std::string label, std::function<void()> on_select = {});
    MenuItem& item(int index) { return items_[static_cast<std::size_t>(index)]; }
    int count() const { return static_cast<int>(items_.size()); }
    void clear() { items_.clear(); selected_ = 0; }

    // Anchor point: the top of the first item; x is the left edge / centre / right edge
    // according to `align`.
    void set_position(Vec2 anchor, Align align = Align::Center) {
        anchor_ = anchor;
        align_ = align;
    }
    int selected() const { return selected_; }
    void set_selected(int index);

    // Call from Scene::update.
    void update(const Input& input);
    void render(Renderer& renderer) const;
    // Screen rectangle of an item (for custom decorations).
    RectF item_rect(int index) const;

    std::function<void()> on_cancel;              // Esc
    std::function<void(int index)> on_change;     // selection moved

private:
    float item_height() const;
    void move(int dir);

    const Theme* theme_;
    std::vector<MenuItem> items_;
    Vec2 anchor_{640.0f, 300.0f};
    Align align_ = Align::Center;
    int selected_ = 0;
};

}  // namespace vs
