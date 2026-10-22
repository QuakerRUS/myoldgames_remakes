#pragma once
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <vs/engine/math.hpp>
#include <vs/engine/text_layout.hpp>

namespace vs {

class Renderer;

enum class Align { Left, Center, Right };

struct FontOptions {
    // The glyph atlas is rasterized at size * oversample pixels and drawn scaled down, so text
    // stays sharp when the 1280x720 canvas is scaled up to a bigger window. Use 1 for pixel fonts.
    float oversample = 2.0f;
    // Linear filtering (true) or nearest (false, for pixel fonts drawn at integer sizes).
    bool smooth = true;
    // Extra code points to include besides the default set (see engine/README.md).
    std::vector<char32_t> extra_chars;
};

struct TextStyle {
    Color color{255, 255, 255, 255};
    Align align = Align::Left;        // which point of each line `pos.x` is: left end, centre, right end
    float max_width = 0.0f;           // > 0: word wrap at this width
    float line_spacing = 1.0f;        // multiplier of line_height()
    std::optional<Color> outline;     // outline colour (drawn at 8 offsets)
    float outline_width = 1.0f;
    std::optional<Color> shadow;      // drop shadow colour
    Vec2 shadow_offset{2.0f, 2.0f};
};

// A TrueType font at one size, rendered from a glyph atlas built with stb_truetype.
// Covers ASCII, Latin-1, Cyrillic U+0400..U+04FF and common punctuation by default.
// Missing glyphs are drawn as '?'. Move-only; must not outlive the Renderer.
class Font {
public:
    Font();
    ~Font();
    Font(Font&&) noexcept;
    Font& operator=(Font&&) noexcept;

    // `size` is the pixel height (ascent - descent) in logical pixels. Throws vs::Error.
    static Font load(Renderer& renderer, const std::filesystem::path& ttf_path, float size,
                     const FontOptions& options = {});

    bool valid() const;
    float size() const;
    float ascent() const;       // baseline offset from the top of a line
    float line_height() const;  // distance between baselines (ascent - descent + line gap)
    bool has_glyph(char32_t cp) const;

    // Width of the widest line of `text` ('\n' separates lines).
    float measure(std::string_view text) const;
    // Size of the block drawn by draw() with the same style (wrapping included).
    Vec2 measure_block(std::string_view text, const TextStyle& style = {}) const;
    // Word wrap with this font's metrics (see wrap_text_ranges).
    std::vector<std::string> wrap(std::string_view text, float max_width) const;
    std::vector<LineRange> wrap_ranges(std::string_view text, float max_width) const;
    // Pen x offset of the code point with byte offset `byte_offset` within a single line
    // (for carets and highlighting a letter).
    float offset_of(std::string_view line, std::size_t byte_offset) const;
    MeasureFn measure_fn() const;

    // Draws text with its first line's top at pos.y; pos.x is the anchor given by style.align.
    // Returns the drawn block size.
    Vec2 draw(Renderer& renderer, std::string_view text, Vec2 pos, const TextStyle& style = {}) const;
    // Draws text aligned horizontally inside `box` by style.align and centred vertically.
    Vec2 draw_in_box(Renderer& renderer, std::string_view text, RectF box, const TextStyle& style = {}) const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace vs
