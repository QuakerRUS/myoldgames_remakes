#pragma once
#include <filesystem>
#include <functional>
#include <string>

#include <vs/editor/confirm_prompt.hpp>

namespace vs {

// What to do once the document is saved (or the user chose not to save).
enum class AfterSave { Nothing, New, Open, Quit };

// The New / Open / Save / Save As / Quit state machine of a document editor (TileEditor,
// Painter): "Unsaved changes. Save?" before New/Open/Quit, Save falling back to Save As for an
// untitled document, and the pending action continued only after a successful save.
// The editor provides the document-specific parts as hooks:
//
//   vs::SaveFlow flow{prompt_, {
//       .modified = [this] { return !undo_.is_clean(); },
//       .path = [this] { return path_; },
//       .choose_path = [this](auto done) { dialogs_.save_file(window_, filters, suggestion, done); },
//       .write = [this](const std::filesystem::path& p) { return save_to(p); },
//       .run = [this](vs::AfterSave a) { ... new document / open dialog / quit ... },
//   }};
//   flow.ask_save_then(vs::AfterSave::Open);   // Ctrl+O
class SaveFlow {
public:
    using PathCallback = std::function<void(const std::filesystem::path&)>;
    struct Hooks {
        std::function<bool()> modified;                // unsaved changes?
        std::function<std::filesystem::path()> path;   // the document's file; empty = untitled
        // Shows a Save As dialog and calls `done` with the chosen path (empty = cancelled); may
        // call it later (asynchronous dialogs).
        std::function<void(PathCallback done)> choose_path;
        // Optional: asked before every write; call proceed(true) to write, proceed(false) to
        // cancel (the pending action is dropped). May ask the user first (asynchronous).
        std::function<void(const std::filesystem::path& target, std::function<void(bool)> proceed)> confirm_write;
        // Writes the document to `target`; shows its own error message; false on failure.
        std::function<bool(const std::filesystem::path& target)> write;
        // Performs New / Open / Quit.
        std::function<void(AfterSave)> run;
        // Optional: the "Save?" question; default "The document has unsaved changes. Save?".
        std::function<std::string()> question;
    };

    SaveFlow(ConfirmPrompt& prompt, Hooks hooks) : prompt_(prompt), hooks_(std::move(hooks)) {}
    SaveFlow(const SaveFlow&) = delete;
    SaveFlow& operator=(const SaveFlow&) = delete;

    // Saves to path() (Save As when untitled), then continues the pending action.
    void save();
    // Save As dialog, then write and continue the pending action; cancelling drops it.
    void save_as();
    // Writes to `target` (after confirm_write), then continues the pending action.
    void save_to(const std::filesystem::path& target);
    // Runs `action` at once if unmodified; otherwise asks "Save?": Yes saves first (the action
    // runs after a successful save), No runs it without saving, Cancel does nothing.
    void ask_save_then(AfterSave action);
    // Window close / Esc: true = may quit right now (unmodified); otherwise asks as
    // ask_save_then(AfterSave::Quit) does and returns false (run(Quit) follows the answer).
    bool request_quit();

    // The action waiting for the current save (Nothing when no save is in progress).
    AfterSave pending() const { return pending_; }
    void cancel_pending() { pending_ = AfterSave::Nothing; }

private:
    void continue_after_save();

    ConfirmPrompt& prompt_;
    Hooks hooks_;
    AfterSave pending_ = AfterSave::Nothing;
};

}  // namespace vs
