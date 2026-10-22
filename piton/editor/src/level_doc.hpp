#pragma once
// SDL-free part of the Piton map editor: the mapping between piton::Level and the generic
// vs::TileDocument edited by the UI, painting with symmetry, the whole-level tools and the
// file I/O (exactly the bytes piton_core writes).
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include <piton/level.hpp>
#include <vs/editor/tile_document.hpp>

namespace piton_editor {

// Grid values (what the TileDocument stores per cell).
inline constexpr int kEmpty = 0;
inline constexpr int kBrick = 1;
// Palette-only codes: "snake start facing <dir>" = kStartBase + int(dir) (11 Up .. 14 Left).
// They are never stored in the grid; placing one moves the level's start.
inline constexpr int kStartBase = 10;

inline int startCode(piton::Dir d) { return kStartBase + static_cast<int>(d); }
std::optional<piton::Dir> startCodeDir(int code);

// Start position of a new level: the middle of the field, facing right.
inline constexpr piton::Cell kNewLevelStart{19, 11};

// Document <-> level. Properties: {"name": string, "start": {"x", "y", "dir": "right"}}.
vs::TileDocument toDocument(const piton::Level& level);
piton::Level toLevel(const vs::TileDocument& doc);
vs::TileDocument newDocument();

std::string docName(const vs::TileDocument& doc);
piton::Cell docStart(const vs::TileDocument& doc);
piton::Dir docStartDir(const vs::TileDocument& doc);
void setDocName(vs::TileDocument& doc, const std::string& name);
void setDocStart(vs::TileDocument& doc, piton::Cell head, piton::Dir dir);

// Symmetry painting: every placed cell is mirrored across the vertical axis (Horizontal),
// the horizontal axis (Vertical) or both.
enum class Symmetry { None, Horizontal, Vertical, Both };
const char* symmetryName(Symmetry s);
bool mirrorsX(Symmetry s);  // Horizontal or Both
bool mirrorsY(Symmetry s);  // Vertical or Both
Symmetry makeSymmetry(bool x, bool y);
// `c` and its mirror images (distinct cells, `c` first).
std::vector<piton::Cell> symmetricCells(piton::Cell c, Symmetry s);
// Grid changes for painting `value` (kEmpty/kBrick) at `c` with symmetry; unchanged cells
// omitted. Bricks are never painted on the start snake's cells.
std::vector<vs::CellChange> paintChanges(const vs::TileDocument& doc, piton::Cell c, int value, Symmetry s);

// Whole-document tools (each returns the new document; the UI records one undo step).
// Moves the start; the bricks under the three start cells are removed. A head too close to
// the edge for the body is moved inwards (piton::Level::clampStart): the start snake always
// lies inside the field.
vs::TileDocument withStart(const vs::TileDocument& doc, piton::Cell head, piton::Dir dir);
vs::TileDocument cleared(const vs::TileDocument& doc);        // all cells empty, start kept
// Bricks on the outer ring (and in the mirror tools below): never on the start snake's cells.
vs::TileDocument borderFilled(const vs::TileDocument& doc);
vs::TileDocument flippedX(const vs::TileDocument& doc);       // mirror left<->right (start too)
vs::TileDocument flippedY(const vs::TileDocument& doc);       // mirror top<->bottom (start too)
vs::TileDocument mirroredLeftToRight(const vs::TileDocument& doc);  // copy left half onto the right
vs::TileDocument mirroredTopToBottom(const vs::TileDocument& doc);  // copy top half onto the bottom

// Files: load throws vs::Error; save writes Level::toJson() atomically.
vs::TileDocument loadDocument(const std::filesystem::path& file);
void saveDocument(const std::filesystem::path& file, const vs::TileDocument& doc);

// First "NN.json" (01..99) that does not exist in `dir`.
std::string suggestFileName(const std::filesystem::path& dir);

// --roundtrip self-check: every *.json in `dir` is loaded and saved through the editor's
// document model; the saved bytes must equal the original file.
struct RoundTripReport {
    int files = 0;
    int failures = 0;
    std::vector<std::string> lines;  // one per file
};
RoundTripReport roundTrip(const std::filesystem::path& dir);

}  // namespace piton_editor
