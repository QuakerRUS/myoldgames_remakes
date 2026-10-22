#pragma once
// Frame read-back for automated visual checks (--screenshot): render, then save what is on the
// render target before it is presented.
#include <filesystem>
#include <optional>

#include <vs/engine/image.hpp>

namespace vs {

class Renderer;

// Reads back the current render target (the whole window, in output pixels). `opaque` forces
// alpha to 255 (the window itself is opaque; ImGui leaves translucent alpha behind).
// nullopt on failure.
std::optional<Image> read_frame(Renderer& renderer, bool opaque = true);

// read_frame + save_png (atomic). Returns false (and logs the reason) on failure; never throws.
// Call it at the end of Scene::render, or let App do it: AppConfig::screenshot_path.
bool save_frame_png(Renderer& renderer, const std::filesystem::path& path, bool opaque = true);

}  // namespace vs
