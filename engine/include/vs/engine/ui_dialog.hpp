#pragma once
#include <functional>
#include <string>
#include <vector>

#include <vs/engine/input.hpp>
#include <vs/engine/renderer.hpp>
#include <vs/engine/ui_theme.hpp>

namespace vs {

enum class DialogResult { Yes, No, Cancel, Ok };
enum class DialogButtons { Ok, YesNo, YesNoCancel };

// Modal message box with Ok / Yes-No / Yes-No-Cancel buttons drawn centred over a dimmed
// screen. Keyboard: Left/Right/Tab move, Enter/Space confirm, Y/N hotkeys (first ASCII letter
// of each label), Esc = Cancel (or No without a Cancel button, or Ok). Mouse: hover + click.
class Dialog {
public:
    explicit Dialog(const Theme* theme) : theme_(theme) {}

    // Labels (e.g. Russian "Да"/"Нет"/"Отмена" for Keyboard Simulator).
    std::string yes_label = "YES";
    std::string no_label = "NO";
    std::string cancel_label = "CANCEL";
    std::string ok_label = "OK";
    float width = 560.0f;
    Vec2 screen_size{1280.0f, 720.0f};  // the dialog is centred on this area

    // Shows the dialog; `on_close` is called once with the choice, after which is_open() is false.
    void open(std::string message, DialogButtons buttons, std::function<void(DialogResult)> on_close,
              DialogResult default_choice = DialogResult::Yes);
    void close() { open_ = false; }
    bool is_open() const { return open_; }

    // Call from Scene::update while is_open() (and do not pass the input to other widgets).
    void update(const Input& input);
    void render(Renderer& renderer) const;

private:
    struct Choice {
        DialogResult result;
        const std::string* label;
    };
    std::vector<Choice> choices() const;
    RectF panel_rect() const;
    RectF button_rect(int index, int count) const;
    void finish(DialogResult r);

    const Theme* theme_;
    std::string message_;
    DialogButtons buttons_ = DialogButtons::Ok;
    std::function<void(DialogResult)> on_close_;
    int selected_ = 0;
    bool open_ = false;
};

}  // namespace vs
