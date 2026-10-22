#pragma once
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace vs {

// An undoable edit. apply() performs (or re-performs) it, revert() takes it back.
class Command {
public:
    virtual ~Command() = default;
    virtual void apply() = 0;
    virtual void revert() = 0;
    // Short description for menus ("Place tile").
    virtual std::string label() const { return {}; }
    // Tries to absorb `next` (already applied) into this command, e.g. consecutive cells of
    // one brush stroke. Return true if merged; `next` is then destroyed.
    virtual bool merge(const Command& next) {
        (void)next;
        return false;
    }
    // Approximate memory the command keeps (e.g. stored pixels), for UndoStack's byte budget.
    // Asked once when the command is recorded and again after a merge. 0 = negligible.
    virtual std::size_t memory_bytes() const { return 0; }
};

// Linear undo/redo history with a "clean" mark for unsaved-changes detection.
// Two limits drop the oldest commands: a count (`limit`, 0 = unlimited) and a byte budget over
// Command::memory_bytes() (set_byte_limit, 0 = unlimited; the newest command is always kept,
// even if it alone exceeds the budget).
class UndoStack {
public:
    explicit UndoStack(std::size_t limit = 500) : limit_(limit) {}
    UndoStack(std::size_t limit, std::size_t byte_limit) : limit_(limit), byte_limit_(byte_limit) {}

    void set_byte_limit(std::size_t bytes);
    std::size_t byte_limit() const { return byte_limit_; }
    // Sum of memory_bytes() of the recorded commands (undo and redo side).
    std::size_t bytes() const { return bytes_; }

    // Applies `cmd` and records it. Drops the redo history. If allow_merge is true and the
    // previous push was also allowed to merge (no break_merge(), undo() or redo() since),
    // the top command's merge() is offered the new one.
    void push(std::unique_ptr<Command> cmd, bool allow_merge = false);
    // Ends a merge sequence (e.g. mouse button released at the end of a stroke).
    void break_merge() { merge_open_ = false; }

    bool can_undo() const { return pos_ > 0; }
    bool can_redo() const { return pos_ < commands_.size(); }
    // Reverts the last command; false if there is nothing to undo.
    bool undo();
    // Re-applies the next command; false if there is nothing to redo.
    bool redo();
    std::string undo_label() const;
    std::string redo_label() const;

    // Forgets the history; the current state becomes clean.
    void clear();
    // Marks the current state as saved.
    void set_clean();
    // True when the state equals the one marked by set_clean() (initially: the empty history).
    bool is_clean() const;
    std::size_t size() const { return commands_.size(); }
    std::size_t position() const { return pos_; }

private:
    void drop_oldest();
    void enforce_limits();

    std::vector<std::unique_ptr<Command>> commands_;
    std::vector<std::size_t> sizes_;  // memory_bytes() of commands_[i]
    std::size_t bytes_ = 0;
    std::size_t byte_limit_ = 0;
    std::size_t pos_ = 0;
    std::size_t limit_;
    long long clean_ = 0;  // index of the clean state; -1 when it can no longer be reached
    bool merge_open_ = false;
};

}  // namespace vs
