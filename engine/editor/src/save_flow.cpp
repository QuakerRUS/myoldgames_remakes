#include <vs/editor/save_flow.hpp>

namespace vs {

void SaveFlow::save() {
    const std::filesystem::path p = hooks_.path ? hooks_.path() : std::filesystem::path();
    if (p.empty()) {
        save_as();
        return;
    }
    save_to(p);
}

void SaveFlow::save_as() {
    if (!hooks_.choose_path) {
        pending_ = AfterSave::Nothing;
        return;
    }
    hooks_.choose_path([this](const std::filesystem::path& chosen) {
        if (chosen.empty()) {
            pending_ = AfterSave::Nothing;
            return;
        }
        save_to(chosen);
    });
}

void SaveFlow::save_to(const std::filesystem::path& target) {
    auto write = [this, target](bool proceed) {
        if (!proceed) {
            pending_ = AfterSave::Nothing;
            return;
        }
        if (hooks_.write && hooks_.write(target))
            continue_after_save();
        else
            pending_ = AfterSave::Nothing;
    };
    if (hooks_.confirm_write)
        hooks_.confirm_write(target, write);
    else
        write(true);
}

void SaveFlow::continue_after_save() {
    const AfterSave a = pending_;
    pending_ = AfterSave::Nothing;
    if (a != AfterSave::Nothing && hooks_.run) hooks_.run(a);
}

void SaveFlow::ask_save_then(AfterSave action) {
    if (!hooks_.modified || !hooks_.modified()) {
        if (action != AfterSave::Nothing && hooks_.run) hooks_.run(action);
        return;
    }
    prompt_.title = "Unsaved changes";
    std::string question = hooks_.question ? hooks_.question() : std::string();
    if (question.empty()) question = "The document has unsaved changes. Save?";
    prompt_.open(std::move(question), [this, action](PromptChoice c) {
        if (c == PromptChoice::Yes) {
            pending_ = action;
            save();
        } else if (c == PromptChoice::No) {
            if (action != AfterSave::Nothing && hooks_.run) hooks_.run(action);
        }
    });
}

bool SaveFlow::request_quit() {
    if (!hooks_.modified || !hooks_.modified()) return true;
    ask_save_then(AfterSave::Quit);
    return false;
}

}  // namespace vs
