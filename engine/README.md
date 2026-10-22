# vs_engine — shared engine of the remakes

A small C++20 library on SDL3 used by Soldier 2.0, Piton, Keyboard Simulator, the map editors
and Painter. Everything is in namespace `vs`. Headers: `#include <vs/engine/<module>.hpp>`
(or the umbrella `<vs/engine/engine.hpp>`).

| CMake target | What | Link it from |
|---|---|---|
| `vs_engine` | app, gfx, text, audio, input, ui, storage, screenshot, window icon, console (+ SDL-free helpers) | games, tools |
| `vs_imgui` | Dear ImGui v1.93.0 (SDL3 + SDL_Renderer backends, `imgui_stdlib`) + `vs::ImGuiLayer` | editors, Painter |
| `vs_editor` | tile map editor base (`vs::TileEditor`, plugin interface, grid view, dialogs, `vs::SaveFlow`) | map editors, Painter |
| `nlohmann_json::nlohmann_json` | JSON (v3.12.0); available even with `VS_LOGIC_ONLY=ON` | anyone |
| `doctest::doctest` | unit tests; use `vs_add_test(<name> <sources...>)` | tests |

Pinned dependencies (`cmake/Dependencies.cmake`): SDL `release-3.4.18`, doctest `v2.5.3`,
nlohmann/json `v3.12.0` (release tarball, SHA-256), stb commit `a7d4c217` (zip, SHA-256),
Dear ImGui `v1.93.0` (tarball, SHA-256).

Conventions:
- Coordinates are **logical pixels** of a 1280×720 canvas (letterboxed into the resizable
  window), y down. Editors switch the canvas off and work in window pixels.
- Resource loaders **throw `vs::Error`** (a `std::runtime_error`) with the file name and reason.
  An exception escaping a scene ends `App::run()` with exit code 1 and a message box.
- Strings are UTF-8 everywhere (paths too: use `vs::utf8_to_path` / `vs::path_to_utf8`).
- Textures, fonts and sprite sheets belong to the renderer: keep them in scenes (or other
  objects that die before the `App`), never in globals.

## CMake helpers

```cmake
add_executable(soldier WIN32 src/main.cpp ... res/app.rc)
target_link_libraries(soldier PRIVATE vs_engine soldier_core)
vs_set_output_folder(soldier Soldier)          # exe -> bin/<Config>/Soldier/, SDL3.dll copied there
vs_add_game_data(soldier ${CMAKE_SOURCE_DIR}/soldier/data          # data/ -> bin/<Config>/Soldier/data,
                 EXTRA ${CMAKE_SOURCE_DIR}/shared/branding branding) # defines VS_DEV_DATA_DIR="<abs path>"
vs_add_test(soldier_core_tests test_main.cpp test_rules.cpp)     # doctest exe + CTest; WORKING_DIRECTORY = repo root
target_link_libraries(soldier_core_tests PRIVATE soldier_core)
```

The helpers live in `cmake/Programs.cmake`. Every program has its own output folder, so programs
with equally named data files (Soldier and Piton both have `gfx/tiles.json`) never overwrite each
other's copies:

| Folder (`build/<preset>/bin/<Config>/...`) | Contents |
|---|---|
| `Soldier/` | `soldier.exe`, `soldier_editor.exe`, `SDL3.dll`, `data/` (with `data/branding/`) |
| `Piton/` | `piton.exe`, `piton_editor.exe`, `SDL3.dll`, `data/` (with `data/branding/`) |
| `KeyboardSimulator/` | `keyboard_simulator.exe`, `SDL3.dll`, `data/` |
| `Painter/` | `painter.exe`, `SDL3.dll` |
| `.` | test executables, `engine-demo.exe`, `SDL3.dll` |

- `vs_set_output_folder(<target> <folder>)`: sets the target's `RUNTIME_OUTPUT_DIRECTORY` to
  `bin/<Config>/<folder>/` and calls `vs_copy_runtime`. A map editor goes into its game's folder,
  so F5 finds the game exe next to it.
- `vs_copy_runtime(<target>)`: copies `SDL3.dll` into the target's folder (one custom target
  `vs-runtime-<folder>` per folder); a no-op for targets in the default `bin/<Config>/`.
- `vs_add_game_data(<target> <data_dir> [EXTRA <src_dir> <data_subdir>]...)`: mirrors the data
  before the target is built (one custom target `vs-data-<folder>` per folder, so two programs of
  a folder never copy at the same time; `cmake/SyncData.cmake` copies changed files and removes
  files deleted or renamed in the sources, so do not keep hand-made files in
  `bin/<Config>/<folder>/data`) and defines `VS_DEV_DATA_DIR`: the absolute source data folder in
  Debug / RelWithDebInfo / MinSizeRel, `""` in Release, so shipped exes do not embed the
  developer's path and find their assets only in `<exe_dir>/data`. The first call for a folder
  defines its data; later calls name the same `<data_dir>` and may omit `EXTRA`.

Packaging (`cmake/Install.cmake`): `cmake --install build/vs2022 --config Release --prefix dist`
installs every folder as a self-contained package (`dist/Soldier 2.0/`, `dist/Piton/`,
`dist/Keyboard Simulator/`, `dist/Painter/`: the exes, `SDL3.dll`, the Visual C++ runtime DLLs,
`data/` and the `<project>/package/README.txt`); `tools/package.ps1` builds Release, installs and
zips each folder. `vs_install_folder(<folder> <package> <readme>)` adds a package.

A doctest main file is one line pair:
```cpp
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
```
(`DOCTEST_CONFIG_USE_STD_HEADERS` is defined for every doctest user: MSVC needs it for
`CHECK(string_view == "...")`.)

## Smoke tests (no visible window)

Every program supports `--smoke` (via `vs::parse_common_args`): it starts, runs one second
and exits with 0. Run smoke tests **without opening windows or using the sound card**:

```sh
# Git Bash
SDL_VIDEO_DRIVER=offscreen SDL_AUDIO_DRIVER=dummy build/vs2022/bin/Debug/Soldier/soldier.exe --smoke; echo $?
```
```powershell
# PowerShell
$env:SDL_VIDEO_DRIVER='offscreen'; $env:SDL_AUDIO_DRIVER='dummy'; .\build\vs2022\bin\Debug\Soldier\soldier.exe --smoke; $LASTEXITCODE
```
(The offscreen driver prints a harmless "Installed Vulkan doesn't implement
VK_EXT_headless_surface" line.)

Screenshots for visual checks: set `AppConfig::screenshot_path` (+ `screenshot_after_frames`) and
the App saves that frame and quits, or call `vs::save_frame_png(renderer, path)` from a scene when
the shot depends on scene state (see "screenshot" below). Command-line modes of WIN32 programs
call `vs::attach_parent_console()` first so their output shows up in the terminal.

`tools/engine-demo` exercises the engine: `engine-demo --smoke`, `engine-demo --editor --smoke`
(demo map editor), and `engine-demo --smoke --screenshot out.png` saves a rendered frame.

---

## app — `app.hpp`, `scene.hpp`

```cpp
struct AppConfig {
    std::string title = "V Software";
    std::string game_id;              // %APPDATA%\V Software\<game_id>
    int window_width = 1280, window_height = 720;
    int logical_width = 1280, logical_height = 720;  // 0 = no logical canvas (editors)
    bool integer_scale = false;       // scale the canvas only by whole multiples
    bool fullscreen = false;          // borderless desktop fullscreen
    bool vsync = true;
    bool resizable = true;
    double logic_step = 1.0 / 60.0;   // seconds per Scene::update
    int max_steps_per_frame = 8;
    bool fullscreen_keys = true;      // Alt+Enter and F11 toggle fullscreen
    bool enable_audio = true;
    Color clear_color{0, 0, 0, 255};  // letterbox bars
    double quit_after = 0.0;          // > 0: quit after N seconds (set by --smoke)
    std::filesystem::path screenshot_path;  // set: save frame no. screenshot_after_frames as PNG, quit
    int screenshot_after_frames = 1;        //      (exit code 0, or 1 if saving failed)
    std::filesystem::path data_dir;   // --data-dir <path>: user_data_dir() returns it
    bool smoke = false;               // --smoke
};
std::vector<std::string> parse_common_args(int argc, char** argv, AppConfig&);
// handles --smoke (quit_after = 1, smoke = true), --data-dir <path> (or --data-dir=<path>),
// --windowed, --fullscreen; returns the other arguments.
// --data-dir calls vs::set_user_data_dir_override(path); --smoke without --data-dir calls
// vs::set_user_data_smoke_mode(true) so user_data_dir() uses %TEMP%/vs-smoke/<game>, never
// %APPDATA% (see "storage").

class App {
    explicit App(AppConfig);
    bool init();                         // false on failure -> error()
    const std::string& error() const;
    int run(std::unique_ptr<Scene> first);   // returns quit() code; 1 on exception
    void quit(int exit_code = 0);
    void push_scene(std::unique_ptr<Scene>);
    void pop_scene();
    void replace_scene(std::unique_ptr<Scene>);
    Scene* top_scene() const;  std::size_t scene_count() const;
    void set_fullscreen(bool);  bool fullscreen() const;  void toggle_fullscreen();
    bool is_fullscreen() const;          // current (requested) state; follows system changes too
    void set_fullscreen_listener(std::function<void(bool)>);  // after every change (see below)
    void set_title(std::string_view);
    void set_integer_scale(bool);
    void set_logic_step(double seconds);
    const AppConfig& config() const;
    double time() const;                 // seconds since run()
    void set_raw_event_hook(std::function<bool(const SDL_Event&)>);  // true = consumed (ImGui)
    Renderer& renderer();  Input& input();  Audio& audio();
    SDL_Window* window() const;  SDL_Renderer* sdl_renderer() const;
};

class Scene {
    virtual void on_enter();             // became top of the stack
    virtual void on_exit();              // stopped being top (popped, replaced, covered)
    virtual void on_event(const SDL_Event&);   // top scene only; mouse coords are logical
    virtual void update(double dt);      // fixed step, dt = logic_step; top scene only
    virtual void render(Renderer&, double alpha);  // once per frame
    virtual bool is_overlay() const;     // true: the scene below renders first
    virtual bool on_quit_request();      // window close / Alt+F4: return false to refuse
    App& app() const;                    // valid from on_enter()
};
```

Main loop per frame: poll events (Alt+Enter/F11 handled by the App; window close asks the top
scene's `on_quit_request()`), then run `update(step)` as many times as the accumulated real
time allows (at most `max_steps_per_frame`), then render the stack from the topmost
non-overlay scene upward with `alpha` = leftover time / step, then present (vsync).
Scene changes requested anywhere are applied after the current callback returns, so
`app().replace_scene(...)` from inside `update()` is safe. `run()` returns when `quit()` is
called or the stack becomes empty.

Window close: the title bar's X and Alt+F4 arrive as `SDL_EVENT_QUIT`, which the App always
routes to the top scene's `on_quit_request()` (also while ImGui has focus; the raw event hook
cannot swallow it). Return `true` to let the App quit, or `false` to refuse and ask first
("Save changes?" / "Quit the game?") and call `app().quit()` once the user confirms. The scene
is asked once per close request.

Fullscreen: `set_fullscreen_listener(fn)` is called with the new state after every change,
whether by `set_fullscreen` / `toggle_fullscreen`, Alt+Enter / F11 (`fullscreen_keys`) or the
system (SDL's enter/leave-fullscreen window events); not when the state stays the same. Games
store the setting from it:
```cpp
app.set_fullscreen_listener([&](bool on) { settings.fullscreen = on; save_settings(settings); });
```
`is_fullscreen()` returns the state the App tracks (`fullscreen()` queries the window flags).

Released keys and mouse buttons (`SDL_EVENT_KEY_UP`, `SDL_EVENT_MOUSE_BUTTON_UP`) always reach
`vs::Input`, even when the raw hook (ImGui) consumes them, so nothing stays "down"; the scene's
`on_event` does not see consumed events.

Interpolation: keep the previous and current logic positions and draw `vs::lerp(prev, cur, (float)alpha)`.
Tick-based games (Soldier: 500 ms tick) accumulate `dt` in `update` and advance their own ticks.

```cpp
class MenuScene : public vs::Scene {
    void update(double) override {
        if (app().input().pressed(SDLK_ESCAPE)) app().quit();
        if (app().input().pressed(SDLK_RETURN)) app().replace_scene(std::make_unique<GameScene>());
    }
    void render(vs::Renderer& r, double) override { r.clear(vs::rgb(0x101820)); }
};

int main(int argc, char** argv) {      // with #include <SDL3/SDL_main.h>
    vs::AppConfig cfg;
    cfg.title = "Soldier 2.0";
    cfg.game_id = "Soldier";
    vs::parse_common_args(argc, argv, cfg);
    vs::init_asset_roots(VS_DEV_DATA_DIR);
    vs::App app(cfg);
    if (!app.init()) return 1;
    return app.run(std::make_unique<MenuScene>());
}
```

## Basic types — `math.hpp`, `error.hpp`

```cpp
struct Vec2 { float x, y; };            // + - * / += -= ==
struct RectF { float x, y, w, h;        // right() bottom() position() size() center()
               bool contains(Vec2) const; RectF inset(float d) const; };
struct Color { uint8_t r, g, b, a = 255; Color with_alpha(uint8_t) const; };
constexpr Color rgb(uint32_t 0xRRGGBB);  constexpr Color rgba(uint32_t 0xRRGGBBAA);
float lerp(float, float, float);  Vec2 lerp(Vec2, Vec2, float);
class Error : public std::runtime_error;
```

## gfx

### Renderer — `renderer.hpp`

```cpp
enum class Flip { None, Horizontal, Vertical, Both };
struct DrawParams {
    Vec2 scale{1, 1};
    Flip flip = Flip::None;
    float rotation = 0;            // degrees clockwise, around the origin
    std::optional<Vec2> origin;    // pivot in source pixels (sprites default to the frame pivot)
    Color tint{255,255,255,255};   // colour modulation, tint.a = alpha
};
class Renderer {
    Vec2 size() const;             // logical size (1280x720) or window pixels
    void clear(Color);
    void fill_rect(RectF, Color);
    void draw_rect(RectF, Color, float thickness = 1);         // outline inside the rect
    void draw_line(Vec2 a, Vec2 b, Color, float thickness = 1);
    void fill_rounded_rect(RectF, float radius, Color);
    void draw_rounded_rect(RectF, float radius, Color, float thickness = 1);
    void fill_circle(Vec2 center, float radius, Color);
    void fill_gradient(RectF, Color top, Color bottom, float radius = 0);
    void draw(const Texture&, std::optional<RectF> src, Vec2 pos, const DrawParams& = {});
    void draw_stretched(const Texture&, std::optional<RectF> src, RectF dst, Color tint = {});
    void set_clip(std::optional<RectF>);
    SDL_Renderer* sdl() const;     // escape hatch
};
```
`draw` places the origin point of `src` on `pos`; the drawn size is `src` size × `scale`.
All primitives alpha-blend (also the geometry-based ones: circles, rounded rectangles, gradients,
thick lines — SDL draws untextured geometry with the renderer's draw blend mode, which they set).

### Screenshot — `screenshot.hpp`

```cpp
std::optional<Image> read_frame(Renderer&, bool opaque = true);   // the render target before present
bool save_frame_png(Renderer&, const path&, bool opaque = true);  // false + SDL_Log on failure, never throws
```
Call it at the end of `Scene::render` (after ImGui's `layer.render()`), or let the App do it with
`AppConfig::screenshot_path`. `opaque` sets alpha to 255 (the window is opaque; ImGui leaves
translucent alpha in the back buffer).

### Window icon — `window_icon.hpp`

```cpp
bool set_window_icon(SDL_Window*, const Image& icon, const std::vector<Image>& alternates = {});
bool set_window_icon(SDL_Window*, const path& png, const std::vector<path>& alternates = {});
bool set_window_icon(App&, const path& png, const std::vector<path>& alternates = {});
// vs::set_window_icon(app, vs::find_asset("gfx/icon.png"), {vs::find_asset("gfx/icon_16.png")});
```
Never throws; missing alternates (empty paths) are skipped. This is the runtime icon of the SDL
window; the icon of the .exe file is a resource, see "Application icons".

### Application icons (.exe resource)

Every program embeds a multi-size icon (16, 24, 32, 48, 64, 256 px) generated by
`tools/assets/gen_icons.py`: `<program>/res/app.ico` plus `res/app.rc` containing
`1 ICON "app.ico"`, listed among the target's sources
(`add_executable(piton-editor WIN32 src/main.cpp ... res/app.rc)`). Explorer shows the first icon
resource, and SDL uses it for the window class, so the title bar has it too.

## console — `console.hpp`

```cpp
void attach_parent_console();   // WIN32 exe: show printf/std::cout in the calling terminal
```
When stdout is not already a file or a pipe, attaches stdout/stderr to the parent process' console
(cmd, PowerShell, Git Bash). Call it at the start of command-line modes (`--roundtrip`, ...).

### Texture and Image — `texture.hpp`, `image.hpp`

```cpp
enum class ScaleMode { Nearest, Linear };      // Nearest is the default (pixel art)
class Texture {                                // move-only
    static Texture load(Renderer&, const std::filesystem::path& png, ScaleMode = Nearest);
    static Texture from_image(Renderer&, const Image&, ScaleMode = Nearest);
    static Texture create_streaming(Renderer&, int w, int h, ScaleMode = Nearest);
    void update(const Image&);                 // same size
    bool valid() const; int width() const; int height() const;
    void set_scale_mode(ScaleMode); SDL_Texture* sdl() const;
};
struct Image {                                 // SDL-free RGBA8, top row first
    int width, height; std::vector<uint8_t> pixels;
    Image(int w, int h, Color fill = {0,0,0,0});
    Color get(int x, int y) const; void set(int x, int y, Color); bool in_bounds(int, int) const;
};
Image load_image(const path&);                 // PNG etc. via stb_image
Image decode_image(const void* data, size_t size);
std::vector<uint8_t> encode_png(const Image&);
void save_png(const path&, const Image&);      // atomic write
```

### Sprite sheets — `sprite_sheet_data.hpp` (SDL-free), `sprite_sheet.hpp`

#### Sprite sheet JSON

A sprite sheet is a PNG plus a JSON file next to it:

```json
{
  "image": "soldier.png",
  "frames": {
    "walk_down_0": { "x": 0,  "y": 0, "w": 32, "h": 32 },
    "walk_down_1": { "x": 32, "y": 0, "w": 32, "h": 32, "pivot_x": 16, "pivot_y": 31 }
  },
  "grid": { "frame_w": 32, "frame_h": 32, "columns": 8, "count": 16,
            "prefix": "tile", "margin": 0, "spacing": 0, "pivot_x": 0, "pivot_y": 0 },
  "animations": {
    "walk_down": { "loop": true,
                   "frames": [ { "frame": "walk_down_0", "ms": 120 },
                               { "frame": "walk_down_1", "ms": 120 } ] },
    "water":     { "loop": true, "ms": 200, "frames": ["tile3", "tile4", "tile5"] }
  }
}
```

- `image` (required): PNG file name relative to the JSON file.
- `frames` (optional object): name → rectangle in image pixels; `pivot_x/pivot_y` (default 0,0)
  is the point, relative to the frame's top-left corner, that is placed at the draw position.
- `grid` (optional): generates `count` frames named `<prefix><index>` (`prefix` default
  `"frame"`), row by row with `columns` per row: `x = margin + (i % columns) * (frame_w + spacing)`,
  `y = margin + (i / columns) * (frame_h + spacing)`. Grid frames come first, then `frames`.
- `animations` (optional object): `frames` is a non-empty array whose entries are either a
  frame name (duration = the animation's `"ms"`, default 100) or `{"frame": name, "ms": n}`;
  `loop` defaults to `true`. Durations are milliseconds > 0.
- Frame names must be unique; animations may only use existing frames; every frame must
  lie inside the image (checked by `SpriteSheet::load`). Order of the file is preserved.

```cpp
struct FrameRect { std::string name; int x, y, w, h, pivot_x, pivot_y; };
struct AnimationFrame { std::string frame; int ms; };
struct AnimationDef { std::string name; std::vector<AnimationFrame> frames; bool loop; int total_ms() const; };
struct SpriteSheetData {
    std::string image; std::vector<FrameRect> frames; std::vector<AnimationDef> animations;
    const FrameRect* find_frame(std::string_view) const;
    const AnimationDef* find_animation(std::string_view) const;
};
SpriteSheetData parse_sprite_sheet(std::string_view json_text);   // throws vs::Error
SpriteSheetData load_sprite_sheet_data(const path& json);
std::string sprite_sheet_to_json_text(const SpriteSheetData&);    // canonical form (Painter saves this)

class SpriteSheet {                      // texture + data, move-only
    static SpriteSheet load(Renderer&, const path& json, ScaleMode = Nearest);
    const Texture& texture() const; const SpriteSheetData& data() const;
    const FrameRect* frame(std::string_view) const;
    const AnimationDef* animation(std::string_view) const;
    void draw(Renderer&, std::string_view frame, Vec2 pos, const DrawParams& = {}) const;
    void draw(Renderer&, const AnimationPlayer&, Vec2 pos, const DrawParams& = {}) const;
    void draw_stretched(Renderer&, std::string_view frame, RectF dst, Color tint = {}) const;
};
```

### Animation player — `animation.hpp` (SDL-free)

```cpp
class AnimationPlayer {
    explicit AnimationPlayer(const AnimationDef*);
    void play(const AnimationDef*, bool restart = false);  // same def + !restart: continues
    void update(double dt_seconds);       // * speed()
    void restart(); void stop();
    void set_speed(double); double speed() const;
    const std::string& frame() const;     // current frame name ("" if none)
    int frame_index() const; bool finished() const;  // finished: non-loop past its end (shows last frame)
    bool playing() const; double elapsed_ms() const; const AnimationDef* animation() const;
};
```
The `AnimationDef` must outlive the player (it lives in the `SpriteSheet`).

```cpp
vs::SpriteSheet hero = vs::SpriteSheet::load(app().renderer(), vs::asset_path("sprites/hero.json"));
vs::AnimationPlayer walk(hero.animation("walk_down"));
walk.update(dt);                                              // in update()
hero.draw(r, walk, cam.world_to_screen(pos), {.scale = {2, 2}, .flip = vs::Flip::Horizontal});
```

### Camera — `camera.hpp` (SDL-free)

```cpp
class Camera2D {
    RectF viewport{0, 0, 1280, 720};  // screen area it draws into
    Vec2 position;                    // world point at the viewport's top-left
    float zoom = 1;                   // screen px per world unit (integers for pixel art)
    bool pixel_snap = true;           // round the scroll offset to whole pixels
    Vec2 view_size() const;  RectF view_rect() const;
    void set_bounds(RectF world); void clear_bounds();
    void center_on(Vec2 target);                              // immediate, then clamp
    void follow(Vec2 target, double dt, float stiffness = 8); // exponential smoothing, then clamp
    void clamp();  // inside bounds; a world smaller than the view is centred on that axis
    Vec2 world_to_screen(Vec2) const; Vec2 screen_to_world(Vec2) const;
};
```

## text — `font.hpp`, `text_layout.hpp`, `utf8.hpp`

```cpp
enum class Align { Left, Center, Right };
struct FontOptions { float oversample = 2; bool smooth = true; std::vector<char32_t> extra_chars; };
struct TextStyle {
    Color color{255,255,255,255};
    Align align = Align::Left;    // pos.x is the left end / centre / right end of every line
    float max_width = 0;          // > 0: word wrap
    float line_spacing = 1;
    std::optional<Color> outline; float outline_width = 1;
    std::optional<Color> shadow;  Vec2 shadow_offset{2, 2};
};
class Font {                      // move-only
    static Font load(Renderer&, const path& ttf, float size_px, const FontOptions& = {});
    float size() const; float ascent() const; float line_height() const;
    bool has_glyph(char32_t) const;
    float measure(std::string_view) const;                       // widest line
    Vec2 measure_block(std::string_view, const TextStyle& = {}) const;
    std::vector<std::string> wrap(std::string_view, float max_width) const;
    std::vector<LineRange> wrap_ranges(std::string_view, float max_width) const;
    float offset_of(std::string_view line, size_t byte_offset) const;  // caret / letter x
    MeasureFn measure_fn() const;
    Vec2 draw(Renderer&, std::string_view, Vec2 pos, const TextStyle& = {}) const;  // pos.y = top
    Vec2 draw_in_box(Renderer&, std::string_view, RectF box, const TextStyle& = {}) const; // v-centred
};
```
- Glyphs are baked with stb_truetype into one atlas: U+0020–007E, U+00A0–00FF, Cyrillic
  U+0400–04FF, U+2010–2027 (dashes, quotes, bullet, ellipsis), № ₽ × ‹ › ← ↑ → ↓ ▲ ▼ ◀ ▶,
  plus `extra_chars`. Missing glyphs draw as `?`.
- `size_px` is the font's pixel height in logical pixels. The atlas is rendered at
  `size × oversample` and drawn scaled down with linear filtering, so text stays sharp in a
  larger window. For pixel fonts use `{.oversample = 1, .smooth = false}`.
- One `Font` = one size; load several for several sizes.

```cpp
vs::Font font = vs::Font::load(r, vs::asset_path("fonts/main.ttf"), 28);
font.draw(r, "Привет, мир!", {640, 40}, {.color = vs::rgb(0xFFE070), .align = vs::Align::Center,
                                        .outline = vs::Color{0, 0, 0, 255}});
```

Word wrap (SDL-free, testable with a fake metric):
```cpp
struct LineRange { size_t begin, end; };   // byte range in the original text
using MeasureFn = std::function<float(std::string_view)>;
std::vector<LineRange> wrap_text_ranges(std::string_view text, float max_width, const MeasureFn&);
std::vector<std::string> wrap_text(std::string_view text, float max_width, const MeasureFn&);
```
Greedy: breaks at `\n` and at spaces; the spaces at a soft break belong to neither line (the
next line starts after them); the last line of a paragraph keeps trailing spaces; a word
wider than the line is split between code points; `max_width <= 0` only breaks at `\n`;
empty text gives one empty line.

UTF-8 helpers:
```cpp
char32_t utf8_next(std::string_view, size_t& pos);  // malformed -> U+FFFD, advances 1 byte
std::u32string utf8_decode(std::string_view);   std::string utf8_encode(std::u32string_view);
void utf8_append(std::string&, char32_t);       size_t utf8_length(std::string_view);
void utf8_pop_back(std::string&);               size_t utf8_offset(std::string_view, size_t index);
char32_t to_upper(char32_t);                    // ASCII and Cyrillic (incl. ё -> Ё)
```

## audio — `audio.hpp`

```cpp
class Sound {                          // shared immutable PCM, cheap to copy
    static Sound load(const path&);    // .wav (SDL_LoadWAV) or .ogg (stb_vorbis); throws vs::Error
    static Sound from_samples(std::vector<int16_t> interleaved, int channels, int sample_rate);  // rate <= 0 throws
    bool valid() const; double duration() const;
};
using VoiceId = uint32_t;              // 0 = none
class Audio {                          // owned by App: app().audio()
    bool available() const;            // false: no device, all calls are no-ops
    VoiceId play(const Sound&, float volume = 1, float pitch = 1, float pan = 0, bool loop = false);
    void stop(VoiceId); void stop_all_sounds(); bool is_playing(VoiceId) const;
    void play_music(const Sound&, bool loop = true, double crossfade_seconds = 0);  // same track: no-op
    void stop_music(double fade_out_seconds = 0); bool music_playing() const;
    void set_music_rate(double);       // resampling ratio: 1.25 = 25% faster (and higher)
    double music_rate() const;
    void set_master_volume(float); void set_sfx_volume(float); void set_music_volume(float);  // 0..1
    float master_volume() const; float sfx_volume() const; float music_volume() const;
    void set_paused(bool);
};
```
Mixing: 48 kHz stereo float on an SDL3 audio stream, up to 32 effect voices (the oldest is
stolen), linear-interpolation resampling (so any source rate and `pitch` work), music in its own
slots for crossfades: a crossfade started while another one runs keeps the louder outgoing
track fading over the new crossfade and fades the quieter one out within 50 ms (nothing is cut
off). Sample rates <= 0 are rejected (loaders throw, `from_samples` throws, the mixer never plays
such a buffer). Effective gain = master × (sfx | music) × per-call volume. Sounds are
decoded fully into memory (int16).

```cpp
vs::Sound step = vs::Sound::load(vs::asset_path("sounds/step.wav"));
app().audio().play(step, 0.8f, 1.0f + 0.05f * jitter);
app().audio().play_music(vs::Sound::load(vs::asset_path("music/game.ogg")), true, 1.0);
app().audio().set_music_rate(time_left < 15 ? 1.25 : 1.0);   // "faster music"
```

## input — `input.hpp`

```cpp
class Input {                          // app().input()
    bool down(SDL_Keycode) const;      // held now
    bool pressed(SDL_Keycode) const;   // went down (no OS auto-repeat)
    bool released(SDL_Keycode) const;
    bool repeated(SDL_Keycode) const;  // pressed or auto-repeated (menus, Backspace)
    bool any_pressed() const;          // any key or mouse button (skip intros)
    SDL_Keymod mods() const; bool ctrl() const; bool shift() const; bool alt() const;
    const std::string& text() const;   // UTF-8 typed text (SDL_EVENT_TEXT_INPUT)
    void start_text_input(); void stop_text_input(); bool text_input_active() const;
    Vec2 mouse() const;                // logical coordinates
    bool mouse_moved() const;
    bool mouse_down(int button) const; bool mouse_pressed(int) const; bool mouse_released(int) const;
    float wheel() const;               // vertical steps, + = away from the user
};
```
Keys are `SDL_Keycode`s (`SDLK_LEFT`, `SDLK_RETURN`, `SDLK_A`, `SDLK_COMMA`, ...).
Edge queries (`pressed`, `released`, `repeated`, `text`, `wheel`, `mouse_pressed/released`,
`mouse_moved`) cover the events since the previous logic step and are true during exactly
**one** `update()` call — no press is lost or doubled when a frame runs 0 or several steps.
Read them in `update()`, not in `render()`. Text arrives only while
`start_text_input()` is active (Keyboard Simulator types by the actual Russian character).
Held keys are released when the window loses focus.

## ui — `ui_theme.hpp`, `ui_menu.hpp`, `ui_text_field.hpp`, `ui_dialog.hpp`, `ui_button.hpp`

All widgets take a `const Theme*` (must outlive them), are updated from `Scene::update` with
`app().input()` and drawn in `Scene::render`.

```cpp
enum class UiSound { Move, Select, Cancel, Type, Error };
struct Theme {
    const Font* font;                 // required
    const Font* title_font;           // dialog message (optional)
    Color text, text_disabled, highlight_text, highlight_bg, panel_bg, panel_border, overlay,
          button_bg, button_hover, button_pressed, button_text, button_border, button_active,
          field_bg, field_border, caret;
    float corner_radius = 8, border_width = 2, padding = 12, item_spacing = 8;
    std::optional<Color> text_shadow, text_outline;
    std::string menu_marker;          // drawn before the selected item, e.g. "> "
    std::function<void(UiSound)> on_sound;   // play click sounds here
};

struct MenuItem { std::string label; std::function<void()> on_select;
                  std::function<void(int dir)> on_adjust; bool enabled = true; };
class Menu {
    explicit Menu(const Theme*);
    int add(std::string label, std::function<void()> on_select = {});
    MenuItem& item(int); int count() const; void clear();
    void set_position(Vec2 anchor, Align = Align::Center);   // top of the first item
    int selected() const; void set_selected(int);
    void update(const Input&); void render(Renderer&) const; RectF item_rect(int) const;
    std::function<void()> on_cancel;           // Esc
    std::function<void(int)> on_change;        // selection moved
};
```
Menu keys: Up/Down (repeat, wraps, skips disabled), Enter/Space select (calls `on_adjust(+1)`
if there is no `on_select`), Left/Right `on_adjust(∓1)`, Esc `on_cancel`; mouse hover selects,
click activates, wheel moves.

```cpp
class TextField {
    explicit TextField(const Theme*);
    std::string text; size_t max_length = 32;      // code points
    bool uppercase = false;                        // applied before the filter
    std::function<bool(char32_t)> filter;          // empty = any printable
    RectF rect; Align align = Align::Left; bool draw_box = true; std::string placeholder;
    std::function<void(const std::string&)> on_submit; std::function<void()> on_cancel;
    void update(const Input&, double dt); void render(Renderer&) const;
    bool insert(std::string_view utf8);
    static bool latin_upper_or_digit(char32_t); static bool digit(char32_t);
};
```
The owner calls `input.start_text_input()` while the field is edited. Backspace (repeat)
deletes the last code point, Enter → `on_submit(text)`, Esc → `on_cancel()`.
Soldier names: `max_length = 8; uppercase = true; filter = vs::TextField::latin_upper_or_digit;`.

```cpp
enum class DialogResult { Yes, No, Cancel, Ok };
enum class DialogButtons { Ok, YesNo, YesNoCancel };
class Dialog {
    explicit Dialog(const Theme*);
    std::string yes_label = "YES", no_label = "NO", cancel_label = "CANCEL", ok_label = "OK";
    float width = 560; Vec2 screen_size{1280, 720};
    void open(std::string message, DialogButtons, std::function<void(DialogResult)> on_close,
              DialogResult default_choice = DialogResult::Yes);
    void close(); bool is_open() const;
    void update(const Input&); void render(Renderer&) const;
};
```
While `is_open()`, update only the dialog. Keys: Left/Right/Tab, Enter/Space, hotkeys = first
ASCII letter of each label (Y/N/C), Esc = Cancel (YesNo: No; Ok: Ok); mouse click.

```cpp
class Button {
    Button(const Theme*, std::string label, RectF rect, std::function<void()> on_click = {});
    std::string label; RectF rect; std::function<void()> on_click;
    SDL_Keycode hotkey = SDLK_UNKNOWN; bool enabled = true; bool active = false;  // toggled look
    void update(const Input&); void render(Renderer&) const;
    bool hovered() const; bool pressed() const;
};
```
Click = left button pressed and released over the button. Colours: `button_bg` /
`button_active` / `button_hover` / `button_pressed`, rounded by `corner_radius`.

## storage — `storage.hpp`, `json_file.hpp`

```cpp
std::filesystem::path user_data_dir(std::string_view game);  // %APPDATA%\V Software\<game>\ (created)
void set_user_data_dir_override(const std::filesystem::path&);  // non-empty: user_data_dir() returns it
const std::filesystem::path& user_data_dir_override();        //   (created); empty clears it
void set_user_data_smoke_mode(bool); bool user_data_smoke_mode();  // temp/vs-smoke/<game>
std::filesystem::path exe_dir();
void init_asset_roots(std::string_view dev_data_dir = {});   // [exe_dir/data, dev_data_dir if it exists]
void set_asset_roots(std::vector<path>); const std::vector<path>& asset_roots();
std::filesystem::path asset_path(std::string_view relative); // first root where it exists; throws
std::filesystem::path find_asset(std::string_view relative); // empty path if missing

std::optional<nlohmann::json> read_json_file(const path&);   // nullopt if missing; throws if invalid
void write_json_file(const path&, const nlohmann::json&, int indent = 2);  // atomic
void write_file_atomic(const path&, std::string_view data);  // <path>.tmp then rename; makes dirs
std::string read_text_file(const path&); std::vector<unsigned char> read_binary_file(const path&);
std::string path_to_utf8(const path&); std::filesystem::path utf8_to_path(std::string_view);
```
User data: `user_data_dir(game)` returns, in this order, the override set by
`set_user_data_dir_override` (`--data-dir <path>`, the same folder for every game id), then in
smoke mode (`--smoke` without `--data-dir`) `std::filesystem::temp_directory_path() / "vs-smoke" /
<game>`, otherwise SDL's `%APPDATA%\V Software\<game>\`. Every returned folder is created
(throws `vs::Error` if it cannot be). Tests and smoke runs therefore never write into the real
profile; pass `--data-dir` to a game to keep its saves/settings elsewhere (portable install,
tests that check saved files).

`write_file_atomic` writes `<path>.tmp`, flushes it (`FlushFileBuffers`), then replaces the target
with `MoveFileExW(REPLACE_EXISTING | WRITE_THROUGH)`, retried a few times on sharing violations
(antivirus); on any failure the `.tmp` is removed, the target is untouched and the `vs::Error`
carries the system's reason.

Assets: the game's `data/` folder is copied next to the exe by `vs_add_game_data` (each program
has its own output folder, see "CMake helpers"), and `<exe_dir>/data` is searched **first**. The
source folder (`VS_DEV_DATA_DIR`) is a development fallback, searched second when it exists (a
file added to the source data but not yet copied by a build). A shipped exe uses `<exe_dir>/data`.

```cpp
const auto file = vs::user_data_dir("Soldier") / "players.json";
nlohmann::json players = vs::read_json_file(file).value_or(nlohmann::json::array());
vs::write_json_file(file, players);
```

## undo — `undo_stack.hpp` (SDL-free, in vs_engine)

```cpp
class Command { virtual void apply() = 0; virtual void revert() = 0;
                virtual std::string label() const; virtual bool merge(const Command& next);
                virtual size_t memory_bytes() const; };   // for the byte budget; default 0
class UndoStack {
    explicit UndoStack(size_t limit = 500);
    UndoStack(size_t limit, size_t byte_limit);           // 0 = unlimited
    void set_byte_limit(size_t); size_t byte_limit() const; size_t bytes() const;
    void push(std::unique_ptr<Command>, bool allow_merge = false);  // applies it
    void break_merge();                  // end of a merge sequence (mouse released)
    bool can_undo() const; bool can_redo() const; bool undo(); bool redo();
    std::string undo_label() const; std::string redo_label() const;
    void clear(); void set_clean(); bool is_clean() const;   // "unsaved changes" = !is_clean()
    size_t size() const; size_t position() const;
};
```
`push` drops the redo history. With `allow_merge`, the new (already applied) command is
offered to the top command's `merge()` if the previous push also allowed merging and nothing
(undo, redo, `break_merge`, `set_clean`) happened in between.
The oldest commands are dropped when there are more than `limit` or when the sum of their
`memory_bytes()` exceeds `byte_limit` (the newest command is always kept). Painter uses
400 steps / 512 MB, with commands reporting the pixels they store.

## imgui — `vs_imgui`: `<vs/imgui/imgui_layer.hpp>`

```cpp
struct ImGuiLayerOptions {
    std::filesystem::path font_path;   // empty: segoeui.ttf (Cyrillic) if present, else ImGui default
    float font_size = 18; bool dark_style = true;
    std::filesystem::path ini_path;    // empty: no layout file
};
class ImGuiLayer {
    bool init(SDL_Window*, SDL_Renderer*, const ImGuiLayerOptions& = {});
    bool attach(App&, const ImGuiLayerOptions& = {});   // init + App raw event hook
    void shutdown();
    bool process_event(const SDL_Event&);   // true if ImGui captures it
    void new_frame(); void render();
    bool wants_mouse() const; bool wants_keyboard() const; bool wants_text_input() const;
    static ImTextureID texture_id(const Texture&);      // for ImGui::Image / AddImage
};
```
`std::string` text fields: `#include <imgui_stdlib.h>` and `ImGui::InputText("Name", &str)`.
Use an `App` with `logical_width = 0` (window pixels). Keyboard navigation of ImGui is off
(arrows/Tab belong to the editors). Keep the layer as a Scene member, then:
`on_enter: layer.attach(app());  render: layer.new_frame(); ...; layer.render();`

## editor — `vs_editor`: `<vs/editor/...>`

### Writing a map editor

Implement `vs::TileEditorPlugin` and call `vs::run_tile_editor`:

```cpp
#include <SDL3/SDL_main.h>
#include <vs/editor/run_tile_editor.hpp>

class SoldierEditor : public vs::TileEditorPlugin {
public:
    std::string title() const override { return "Soldier Map Editor"; }
    std::string game_id() const override { return "Soldier"; }
    void load_resources(vs::Renderer& r) override { tiles_ = vs::SpriteSheet::load(r, vs::asset_path("tiles.json")); }
    const vs::SpriteSheet* icons() const override { return &tiles_; }
    std::vector<vs::TileInfo> palette() const override {
        return {{0, "floor", "floor"}, {1, "wall", "wall"}, {2, "exit", "exit"} /* ... */};
    }
    std::vector<vs::CellChange> place(const vs::TileDocument& d, int x, int y, int code) const override;
    std::vector<std::string> validate(const vs::TileDocument& d) const override;  // "No exit", ...
    bool properties_ui(vs::TileDocument& d) override;   // ImGui::InputInt("Time limit", ...) etc.
    vs::TileDocument new_document() const override { return {vs::TileGrid(20, 15, 0), {{"time", 0}}}; }
    vs::TileDocument load(const std::filesystem::path&) const override;          // throw vs::Error
    void save(const std::filesystem::path&, const vs::TileDocument&) const override;
    bool test_play(const std::filesystem::path& f) override {
        return vs::launch_process({vs::path_to_utf8(vs::exe_dir() / "soldier.exe"), "--test-level",
                                   vs::path_to_utf8(f)});
    }
private:
    vs::SpriteSheet tiles_;
};

int main(int argc, char** argv) { SoldierEditor p; return vs::run_tile_editor(p, argc, argv); }
```

Plugin interface (`tile_editor.hpp`) — everything game-specific:

| Member | Default | Meaning |
|---|---|---|
| `std::string title() const` | — | window title prefix |
| `std::string game_id() const` | — | user data folder (layout ini, test-play file) |
| `void load_resources(Renderer&)` | nothing | load icon sprite sheets |
| `std::vector<TileInfo> palette() const` | — | `TileInfo{int code; std::string name; std::string icon_frame; Color color; std::string group;}` — consecutive entries with the same non-empty `group` form a titled section starting on a new row |
| `int initial_palette_index() const` | 0 | selected palette entry at start |
| `const SpriteSheet* icons() const` | `nullptr` | frames named by `TileInfo::icon_frame`; without one a coloured square with the code is drawn |
| `float cell_size() const` | 32 | screen px per cell at zoom 1 |
| `int erase_value() const` | 0 | Backspace / right button value |
| `int palette_columns() const` | 4 | palette layout (also for arrow navigation) |
| `bool draw_cell(ImDrawList*, int code, ImVec2 min, ImVec2 max) const` | false | custom cell drawing (return true if drawn) |
| `std::vector<CellChange> place(const TileDocument&, int x, int y, int code) const` | `{{x,y,code}}` | cell rule: all cells to change; empty = refuse (e.g. a single player: also clear the old one) |
| `std::vector<std::string> validate(const TileDocument&) const` | none | problems; shown in the side panel; block F5 |
| `bool properties_ui(TileDocument& copy)` | false | ImGui widgets editing a copy; return true when changed → one undo step ("Resize" + refit when the size changed) |
| `TileDocument new_document() const` | — | File > New |
| `std::vector<FileFilter> file_filters() const` | `{{"Level (JSON)", "json"}}` | dialog filters; first extension is appended when missing |
| `std::string default_file_name() const` | `"level.json"` | Save As suggestion, test-play file suffix |
| `TileDocument load(const path&) const` | — | throw `vs::Error` on bad files |
| `void save(const path&, const TileDocument&) const` | — | use `vs::write_json_file` (atomic) |
| `bool test_play(const path& saved)` | false | F5: the editor saves the document to `test_play_file()` first; return false or throw `vs::Error(reason)` when the game cannot start |

Optional hooks for full-featured editors (Soldier and Piton use them; every default keeps the
plain editor above). Hooks taking a `TileEditor&` may use its public API: `document()`,
`apply_document(after, label)` (one undo step, no-op when unchanged), `place_at`, `set_status`,
`status()`, `selected_code()`, `select_index/select_code`, `cursor_x/y()`, `set_cursor`,
`hover_x/y()`, `field_active()`, `check()` (cached), `view()`, `fit_to_window()`, `modified()`,
`palette()`, `message()`, `prompt()`.

| Hook | Default | Use |
|---|---|---|
| `bool draw_field_cell(const CellContext&) const` | false | field cell with neighbour access: `CellContext{draw, doc, x, y, code, min, max, cell, time; int at(x, y, outside = -1)}` (autotiles, shadows, animations) |
| `void draw_cell_overlay(const CellContext&) const` | — | second pass over the visible cells (tall objects, editor-only marks) |
| `int overdraw_rows() const` | 0 | extra rows below the view still drawn (objects reaching upwards) |
| `void draw_field_overlay(TileEditor&, const GridView::Frame&)` | — | multi-cell objects, guides (after the cells, before hover and cursor) |
| `bool draw_hover(TileEditor&, const Frame&, x, y)`, `bool draw_cursor(..., x, y, bool active)` | false | custom hover marker / frame cursor (return true if drawn) |
| `PaletteStyle palette_style() const` | `{}` | `item_height` (0 = square), `spacing_scale`, `fill_width`, `selection_rounding`, `selection_inactive`, `pick_returns_to_field`, `wrap_navigation` |
| `std::string palette_header(bool active) const` | "Palette  [active]" / "Palette  (Tab)" | line above the palette |
| `bool draw_palette_item(ImDrawList*, const TileInfo&, ImVec2 min, ImVec2 max, bool hovered) const` | false | the whole item (background, icon, labels) |
| `void palette_tooltip(const TileInfo&) const` | "code: name" | tooltip of the hovered item |
| `void draw_palette_footer(TileEditor&, const TileInfo& selected)` | "Selected: ..." + separator | below the items |
| `std::optional<TileDocument> place_document(doc, x, y, code) const` | nullopt | placement that changes more than cells (e.g. a start position kept in the properties) |
| `bool paints_on_drag(int code) const` | true | false: placed on click only |
| `std::string place_label(int code) const` | "" = "Place <name>" | undo label |
| `void on_placed(TileEditor&, x, y, code)` | — | after a placement (status messages) |
| `ValidationResult check(const TileDocument&) const` | `{validate(doc), {}}` | `{errors, warnings}`: errors block F5, warnings are only shown |
| `bool errors_block_test_play() const` | true | false: F5 asks "Test-play anyway?" |
| `bool report_errors_on_save() const` | false | true: saving with errors reports them (status + popup) |
| `void draw_properties(TileEditor&)` | `properties_ui` on a copy | own undo labels via `apply_document` |
| `void draw_panels(TileEditor&, SidePanelSlot)` | — | extra sections, slots `AfterProperties` / `AfterCheck` |
| `bool draw_check(TileEditor&, const ValidationResult&)` | false | custom Check section |
| `void level_menu_items(TileEditor&)`, `void draw_menus(TileEditor&)` | — | items in the Level menu; extra top-level menus |
| `void handle_keys(TileEditor&)` | — | extra keys (no Ctrl; not while a popup or text field has the keyboard) |
| `bool view_shortcuts() const` | false | Home = fit, G = grid lines |
| `bool draw_status_bar(TileEditor&)` | false | custom status line |
| `float side_panel_width(float avail) const` | min(320, 40 %) | side panel width |
| `bool wrap_cursor() const` | false | arrows wrap around the field edges |
| `GridPos initial_cursor(const TileDocument&, bool opened) const` | (0,0) | cursor after New / Open |
| `std::filesystem::path default_directory() const` | empty | where Open / Save As start |
| `std::string suggest_file_name(const path& dir) const` | `default_file_name()` | Save As name of a new document |
| `new_document_status()`, `unsaved_changes_question()` | "New document", "The document has unsaved changes. Save?" | texts |
| `std::filesystem::path test_play_file() const` | empty = `user_data_dir(game_id)/testplay_<default_file_name>` | F5 temporary file |

Running it with more control (`run_tile_editor.hpp`):
```cpp
AppConfig tile_editor_app_config(const TileEditorPlugin&);  // 1440x900, no canvas, no audio
struct TileEditorRunOptions { path open; bool layout_ini = true; };
std::vector<std::string> parse_tile_editor_args(const std::vector<std::string>& args, AppConfig&,
        TileEditorRunOptions&, int screenshot_frames = 12);  // <file> | --open <file>, --screenshot <png>
int run_tile_editor(TileEditorPlugin&, const AppConfig&, const TileEditorRunOptions&);

int main(int argc, char** argv) {
    MyPlugin plugin;
    vs::AppConfig cfg = vs::tile_editor_app_config(plugin);
    cfg.fullscreen_keys = false;
    vs::TileEditorRunOptions options;
    vs::parse_tile_editor_args(vs::parse_common_args(argc, argv, cfg), cfg, options);
    return vs::run_tile_editor(plugin, cfg, options);
}
```

Document model (`tile_document.hpp`, SDL-free):
```cpp
class TileGrid { TileGrid(int w, int h, int fill = 0); int width() const; int height() const;
                 bool in_bounds(int x, int y) const; int at(int x, int y) const; void set(int x, int y, int v);
                 void resize(int w, int h, int fill = 0);   // keeps the top-left part
                 void fill(int v); int count(int v) const; const std::vector<int>& cells() const; };
struct TileDocument { TileGrid grid; nlohmann::json properties = object; };
struct CellChange { int x, y, value; };
struct GridPos { int x, y; };
class CellsCommand : public Command;     // (doc, changes, label, stroke id) — same stroke ids merge
class DocumentCommand : public Command;  // (doc, after, label) — whole-document swap
```

Editor behaviour (`vs::TileEditor`, used by `run_tile_editor`): menu bar File/Edit/Level,
the field on the left, palette + Properties + Check on the right, status bar.
Keyboard: **Tab** switches the active panel (field ↔ palette); **arrows** move the frame
cursor (field) or the palette selection; **Space/Enter** places the selected tile at the
cursor (in the palette: picks it and returns to the field); **Backspace/Delete** erases;
**Ctrl+Z**, **Ctrl+Y / Ctrl+Shift+Z** undo/redo; **Ctrl+N/O/S**, **Ctrl+Shift+S**;
**F5** test-play; **Esc** or closing the window → "The document has unsaved changes. Save?"
Yes/No/Cancel (Y/N/Esc) when modified. Mouse: LMB places (dragging paints; one undo step per
stroke), RMB erases, wheel zooms around the pointer, middle button pans; clicking a palette
tile selects it. `run_tile_editor` accepts a file to open (or `--open <file>`), `--screenshot <png>`
and the common flags.

Reusable pieces (for Painter and custom editors):
```cpp
class GridView {                        // grid_view.hpp: zoom/pan canvas in the current ImGui window
    float zoom = 1, min_zoom = 0.25f, max_zoom = 16; ImVec2 pan; bool show_grid_lines = true;
    ImU32 background, grid_line, border;
    struct Frame { ImDrawList* draw; ImVec2 canvas_min, canvas_max, origin; float cell; int cols, rows;
                   bool hovered, hovered_cell; int cell_x, cell_y;
                   ImVec2 cell_min(int x, int y) const; ImVec2 cell_max(int x, int y) const; };
    Frame begin(const char* id, int cols, int rows, float cell_px, ImVec2 size = {0, 0});
    void end(const Frame&);
    bool integer_zoom = false;          // pixel art: wheel/zoom_by step 1,2,3,4,5,6,8..64, whole-pixel pan
    bool fit(int cols, int rows, float cell_px);   // false until the first begin()
    void ensure_visible(int x, int y, float cell_px);
    void zoom_by(int dir, ImVec2 local);           // one step around a canvas-local point
};
float next_zoom_level(float zoom, int dir); float zoom_level_at_most(float zoom);  // integer levels
// palette_grid.hpp (SDL-free): grouped palette layout and arrow navigation (wrapping or clamped)
std::vector<PaletteCell> palette_layout(const std::vector<int>& group_sizes, int columns);
int palette_move(const std::vector<int>& group_sizes, int columns, int index, int dx, int dy, bool wrap);
struct FileFilter { std::string name; std::string pattern; };   // pattern: "png;json", "*"
class FileDialogs {                     // file_dialogs.hpp: native async dialogs
    using Callback = std::function<void(const std::filesystem::path&)>;  // empty path = cancelled
    void open_file(SDL_Window*, std::vector<FileFilter>, const path& default_location, Callback);
    void save_file(SDL_Window*, std::vector<FileFilter>, const path& default_location, Callback);
    bool busy() const; void poll();     // call poll() every frame; callbacks run there
};
bool launch_process(const std::vector<std::string>& args);   // start without waiting
enum class PromptChoice { Yes, No, Cancel };
class ConfirmPrompt { std::string title; void open(std::string message,
                      std::function<void(PromptChoice)>, bool with_cancel = true);
                      bool is_open() const; void draw(); };   // Y/Enter, N, Esc/C
class MessagePopup { std::string title; void open(std::string message); bool is_open() const; void draw(); };

// save_flow.hpp: New / Open / Save / Save As / Quit with "Unsaved changes. Save?"
enum class AfterSave { Nothing, New, Open, Quit };
class SaveFlow {
    struct Hooks {
        std::function<bool()> modified;                         // unsaved changes?
        std::function<path()> path;                             // empty = untitled (Save -> Save As)
        std::function<void(PathCallback done)> choose_path;     // Save As dialog; done({}) = cancelled
        std::function<void(const path&, std::function<void(bool)> proceed)> confirm_write;  // optional
        std::function<bool(const path&)> write;                 // shows its own errors
        std::function<void(AfterSave)> run;                     // New / Open / Quit
        std::function<std::string()> question;                  // optional "Save?" text
    };
    SaveFlow(ConfirmPrompt&, Hooks);
    void save(); void save_as(); void save_to(const path&);
    void ask_save_then(AfterSave);     // unmodified: run now; else Yes = save, then run; No = run
    bool request_quit();               // true = quit now; else asks (run(Quit) after the answer)
    AfterSave pending() const; void cancel_pending();
};
```
`SaveFlow` is the state machine shared by `TileEditor` and Painter: the pending action runs
only after a successful save; a cancelled dialog, a refused `confirm_write` or a failed write
drops it. Painter's `confirm_write` asks before a sheet's companion file of another document
is overwritten.

## Tests

`engine/tests` (doctest, `engine_tests`): UTF-8, word wrap with a fake metric, camera
clamping/follow, sprite sheet JSON parsing and round trip, animation player, undo stack,
JSON storage and atomic writes, PNG encode/decode, the audio mixer (SDL-free core), text
field filtering. `engine/editor/tests` (`editor_tests`): tile grid, cell/document commands,
stroke merging, palette layout and navigation, the save flow. Also covered: `write_file_atomic`
failure cleanup, `user_data_dir` override / smoke mode and `parse_common_args`, the undo byte
budget, crossfade during crossfade and sample-rate rejection.
