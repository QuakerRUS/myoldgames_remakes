#include <doctest/doctest.h>
#include <vs/engine/undo_stack.hpp>

#include <memory>

namespace {
struct Add : vs::Command {
    int& target;
    int amount;
    Add(int& t, int a) : target(t), amount(a) {}
    void apply() override { target += amount; }
    void revert() override { target -= amount; }
    std::string label() const override { return "add"; }
    bool merge(const vs::Command& next) override {
        const auto* n = dynamic_cast<const Add*>(&next);
        if (!n || &n->target != &target) return false;
        amount += n->amount;
        return true;
    }
};
}  // namespace

TEST_CASE("undo stack applies, undoes and redoes") {
    int v = 0;
    vs::UndoStack s;
    s.push(std::make_unique<Add>(v, 1));
    s.push(std::make_unique<Add>(v, 10));
    CHECK(v == 11);
    CHECK(s.undo());
    CHECK(v == 1);
    CHECK(s.can_redo());
    CHECK(s.redo_label() == "add");
    CHECK(s.redo());
    CHECK(v == 11);
    CHECK_FALSE(s.redo());
    CHECK(s.undo());
    CHECK(s.undo());
    CHECK(v == 0);
    CHECK_FALSE(s.undo());
}

TEST_CASE("pushing after undo drops the redo history") {
    int v = 0;
    vs::UndoStack s;
    s.push(std::make_unique<Add>(v, 1));
    s.push(std::make_unique<Add>(v, 2));
    s.undo();
    s.push(std::make_unique<Add>(v, 5));
    CHECK(v == 6);
    CHECK_FALSE(s.can_redo());
    CHECK(s.size() == 2);
}

TEST_CASE("clean state tracking") {
    int v = 0;
    vs::UndoStack s;
    CHECK(s.is_clean());
    s.push(std::make_unique<Add>(v, 1));
    CHECK_FALSE(s.is_clean());
    s.set_clean();
    CHECK(s.is_clean());
    s.push(std::make_unique<Add>(v, 1));
    CHECK_FALSE(s.is_clean());
    s.undo();
    CHECK(s.is_clean());
    s.undo();
    CHECK_FALSE(s.is_clean());
    s.push(std::make_unique<Add>(v, 3));  // the saved state can no longer be reached
    CHECK_FALSE(s.is_clean());
    s.undo();
    CHECK_FALSE(s.is_clean());
}

TEST_CASE("merging consecutive commands") {
    int v = 0;
    vs::UndoStack s;
    s.push(std::make_unique<Add>(v, 1), true);
    s.push(std::make_unique<Add>(v, 2), true);
    s.push(std::make_unique<Add>(v, 3), true);
    CHECK(s.size() == 1);
    s.break_merge();
    s.push(std::make_unique<Add>(v, 4), true);
    CHECK(s.size() == 2);
    CHECK(v == 10);
    s.undo();
    CHECK(v == 6);
    s.undo();
    CHECK(v == 0);
}

TEST_CASE("history limit drops the oldest commands") {
    int v = 0;
    vs::UndoStack s(3);
    for (int i = 0; i < 5; ++i) s.push(std::make_unique<Add>(v, 1));
    CHECK(s.size() == 3);
    while (s.undo()) {
    }
    CHECK(v == 2);
}

namespace {
struct Big : vs::Command {
    int& target;
    std::size_t size;
    Big(int& t, std::size_t s) : target(t), size(s) {}
    void apply() override { ++target; }
    void revert() override { --target; }
    std::size_t memory_bytes() const override { return size; }
    bool merge(const vs::Command& next) override {
        const auto* n = dynamic_cast<const Big*>(&next);
        if (!n) return false;
        size += n->size;
        return true;
    }
};
}  // namespace

TEST_CASE("undo stack byte budget drops the oldest commands") {
    int v = 0;
    vs::UndoStack s(0, 1000);
    for (int i = 0; i < 5; ++i) s.push(std::make_unique<Big>(v, 300));
    CHECK(v == 5);
    CHECK(s.size() == 3);  // 900 bytes fit, 1200 do not
    CHECK(s.bytes() == 900);
    CHECK_FALSE(s.is_clean());  // the clean state (empty history) was dropped
    while (s.undo()) {
    }
    CHECK(v == 2);  // only the 3 kept steps are undone

    // The newest command is always kept, even when it alone exceeds the budget.
    s.redo();
    s.push(std::make_unique<Big>(v, 5000));  // also drops the redo side
    CHECK(s.size() == 1);
    CHECK(s.bytes() == 5000);
    CHECK(s.can_undo());

    // Merges update the byte count; shrinking the limit applies at once.
    vs::UndoStack m(0, 0);
    m.push(std::make_unique<Big>(v, 100), true);
    m.push(std::make_unique<Big>(v, 100), true);
    CHECK(m.size() == 1);
    CHECK(m.bytes() == 200);
    m.push(std::make_unique<Big>(v, 50));
    m.push(std::make_unique<Big>(v, 50));
    CHECK(m.bytes() == 300);
    m.set_byte_limit(120);
    CHECK(m.size() == 2);
    CHECK(m.bytes() == 100);
    m.clear();
    CHECK(m.bytes() == 0);
}
