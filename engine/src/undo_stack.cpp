#include <vs/engine/undo_stack.hpp>

namespace vs {

void UndoStack::push(std::unique_ptr<Command> cmd, bool allow_merge) {
    if (!cmd) return;
    cmd->apply();
    if (pos_ < commands_.size()) {
        for (std::size_t i = pos_; i < sizes_.size(); ++i) bytes_ -= sizes_[i];
        commands_.erase(commands_.begin() + static_cast<std::ptrdiff_t>(pos_), commands_.end());
        sizes_.resize(pos_);
        if (clean_ > static_cast<long long>(pos_)) clean_ = -1;
    }
    if (allow_merge && merge_open_ && pos_ > 0 && clean_ != static_cast<long long>(pos_) &&
        commands_[pos_ - 1]->merge(*cmd)) {
        const std::size_t merged = commands_[pos_ - 1]->memory_bytes();
        bytes_ = bytes_ - sizes_[pos_ - 1] + merged;
        sizes_[pos_ - 1] = merged;
        enforce_limits();
        return;
    }
    const std::size_t size = cmd->memory_bytes();
    commands_.push_back(std::move(cmd));
    sizes_.push_back(size);
    bytes_ += size;
    ++pos_;
    merge_open_ = allow_merge;
    enforce_limits();
}

void UndoStack::set_byte_limit(std::size_t bytes) {
    byte_limit_ = bytes;
    enforce_limits();
}

void UndoStack::drop_oldest() {
    bytes_ -= sizes_.front();
    commands_.erase(commands_.begin());
    sizes_.erase(sizes_.begin());
    if (pos_ > 0) {
        --pos_;
        if (clean_ >= 0) --clean_;  // -1 once the clean state has been dropped
    } else if (clean_ > 0) {
        clean_ = -1;  // the dropped command was a redo step leading to the clean state
    }
}

void UndoStack::enforce_limits() {
    while (limit_ > 0 && commands_.size() > limit_) drop_oldest();
    while (byte_limit_ > 0 && bytes_ > byte_limit_ && commands_.size() > 1 && pos_ > 1) drop_oldest();
}

bool UndoStack::undo() {
    merge_open_ = false;
    if (!can_undo()) return false;
    commands_[--pos_]->revert();
    return true;
}

bool UndoStack::redo() {
    merge_open_ = false;
    if (!can_redo()) return false;
    commands_[pos_++]->apply();
    return true;
}

std::string UndoStack::undo_label() const { return can_undo() ? commands_[pos_ - 1]->label() : std::string{}; }

std::string UndoStack::redo_label() const { return can_redo() ? commands_[pos_]->label() : std::string{}; }

void UndoStack::clear() {
    commands_.clear();
    sizes_.clear();
    bytes_ = 0;
    pos_ = 0;
    clean_ = 0;
    merge_open_ = false;
}

void UndoStack::set_clean() {
    clean_ = static_cast<long long>(pos_);
    merge_open_ = false;
}

bool UndoStack::is_clean() const { return clean_ == static_cast<long long>(pos_); }

}  // namespace vs
