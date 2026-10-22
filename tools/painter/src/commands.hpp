#pragma once
// Undoable edits of a Document (for vs::UndoStack).
#include <optional>
#include <string>

#include <vs/engine/undo_stack.hpp>

#include "document.hpp"
#include "raster.hpp"

namespace painter {

// Approximate memory of stored undo data (for the UndoStack byte budget).
std::size_t image_bytes(const vs::Image& image);
std::size_t sheet_bytes(const std::optional<Sheet>& sheet);

// Pixels of one rectangle: before and after (pencil strokes, fills, pastes, flips...).
class RegionCommand : public vs::Command {
public:
    RegionCommand(Document& doc, IRect rect, vs::Image before, vs::Image after, std::string label);
    void apply() override;
    void revert() override;
    std::string label() const override { return label_; }
    std::size_t memory_bytes() const override;

private:
    Document& doc_;
    IRect rect_;
    vs::Image before_, after_;
    std::string label_;
};

// Sprite-sheet data only (frames, animations, pivots, names).
class SheetCommand : public vs::Command {
public:
    SheetCommand(Document& doc, std::optional<Sheet> before, std::optional<Sheet> after, std::string label);
    void apply() override;
    void revert() override;
    std::string label() const override { return label_; }
    std::size_t memory_bytes() const override;

private:
    Document& doc_;
    std::optional<Sheet> before_, after_;
    std::string label_;
};

// The whole image (size changes) together with the sheet data.
class ImageCommand : public vs::Command {
public:
    ImageCommand(Document& doc, vs::Image before, vs::Image after, std::optional<Sheet> sheet_before,
                 std::optional<Sheet> sheet_after, std::string label);
    void apply() override;
    void revert() override;
    std::string label() const override { return label_; }
    std::size_t memory_bytes() const override;

private:
    Document& doc_;
    vs::Image before_, after_;
    std::optional<Sheet> sheet_before_, sheet_after_;
    std::string label_;
};

}  // namespace painter
