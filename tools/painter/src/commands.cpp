#include "commands.hpp"

namespace painter {

std::size_t image_bytes(const vs::Image& image) { return sizeof(vs::Image) + image.pixels.capacity(); }

std::size_t sheet_bytes(const std::optional<Sheet>& sheet) {
    if (!sheet) return 0;
    std::size_t n = sizeof(Sheet) + sheet->data.image.size();
    for (const vs::FrameRect& f : sheet->data.frames) n += sizeof(f) + f.name.capacity();
    for (const vs::AnimationDef& a : sheet->data.animations) {
        n += sizeof(a) + a.name.capacity();
        for (const vs::AnimationFrame& e : a.frames) n += sizeof(e) + e.frame.capacity();
    }
    return n + 64 * sheet->extra.size();
}

std::size_t RegionCommand::memory_bytes() const { return sizeof(*this) + image_bytes(before_) + image_bytes(after_); }

std::size_t SheetCommand::memory_bytes() const { return sizeof(*this) + sheet_bytes(before_) + sheet_bytes(after_); }

std::size_t ImageCommand::memory_bytes() const {
    return sizeof(*this) + image_bytes(before_) + image_bytes(after_) + sheet_bytes(sheet_before_) + sheet_bytes(sheet_after_);
}

RegionCommand::RegionCommand(Document& doc, IRect rect, vs::Image before, vs::Image after, std::string label)
    : doc_(doc), rect_(rect), before_(std::move(before)), after_(std::move(after)), label_(std::move(label)) {}

void RegionCommand::apply() {
    blit(doc_.image, after_, rect_.x, rect_.y, bounds(doc_.image));
    ++doc_.revision;
}

void RegionCommand::revert() {
    blit(doc_.image, before_, rect_.x, rect_.y, bounds(doc_.image));
    ++doc_.revision;
}

SheetCommand::SheetCommand(Document& doc, std::optional<Sheet> before, std::optional<Sheet> after, std::string label)
    : doc_(doc), before_(std::move(before)), after_(std::move(after)), label_(std::move(label)) {}

void SheetCommand::apply() {
    doc_.sheet = after_;
    ++doc_.sheet_revision;
}

void SheetCommand::revert() {
    doc_.sheet = before_;
    ++doc_.sheet_revision;
}

ImageCommand::ImageCommand(Document& doc, vs::Image before, vs::Image after, std::optional<Sheet> sheet_before,
                           std::optional<Sheet> sheet_after, std::string label)
    : doc_(doc),
      before_(std::move(before)),
      after_(std::move(after)),
      sheet_before_(std::move(sheet_before)),
      sheet_after_(std::move(sheet_after)),
      label_(std::move(label)) {}

void ImageCommand::apply() {
    doc_.image = after_;
    doc_.sheet = sheet_after_;
    ++doc_.revision;
    ++doc_.sheet_revision;
}

void ImageCommand::revert() {
    doc_.image = before_;
    doc_.sheet = sheet_before_;
    ++doc_.revision;
    ++doc_.sheet_revision;
}

}  // namespace painter
