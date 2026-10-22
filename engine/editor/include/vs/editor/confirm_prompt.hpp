#pragma once
#include <functional>
#include <string>

namespace vs {

enum class PromptChoice { Yes, No, Cancel };

// ImGui modal "Unsaved changes. Save?" with Yes / No / Cancel.
// Keys: Y or Enter = Yes, N = No, Esc or C = Cancel.
//   prompt.open("Unsaved changes. Save?", [](PromptChoice c) { ... });
//   prompt.draw();   // every frame, inside the ImGui frame
class ConfirmPrompt {
public:
    std::string title = "Unsaved changes";
    void open(std::string message, std::function<void(PromptChoice)> on_choice, bool with_cancel = true);
    bool is_open() const { return open_; }
    void draw();

private:
    std::string message_;
    std::function<void(PromptChoice)> on_choice_;
    bool with_cancel_ = true;
    bool open_ = false;
    bool request_open_ = false;
    int opened_frame_ = 0;
};

// ImGui modal with a message and an OK button (Enter/Esc close it).
class MessagePopup {
public:
    std::string title = "Message";
    void open(std::string message);
    bool is_open() const { return open_; }
    void draw();

private:
    std::string message_;
    bool open_ = false;
    bool request_open_ = false;
    int opened_frame_ = 0;
};

}  // namespace vs
