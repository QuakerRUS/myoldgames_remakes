#pragma once
#include <SDL3/SDL_events.h>

namespace vs {

class App;
class Renderer;

// One screen of a program (intro, menu, game, dialog overlay...). Scenes live on App's stack;
// only the top scene receives events and update(). Rendering starts at the topmost scene
// that is not an overlay and goes up, so overlays (pause, dialogs) draw over the scene below.
class Scene {
public:
    virtual ~Scene() = default;

    // Called when the scene becomes the top of the stack (after push/replace, or when the
    // scene above it is popped).
    virtual void on_enter() {}
    // Called when the scene stops being the top (popped, replaced, or covered by a push).
    virtual void on_exit() {}
    // Raw SDL event (mouse coordinates already in logical pixels). Input state is updated too.
    virtual void on_event(const SDL_Event& event) { (void)event; }
    // Fixed logic step; dt = AppConfig::logic_step seconds.
    virtual void update(double dt) { (void)dt; }
    // Once per frame. alpha in [0, 1) = how far real time is between the previous and the
    // next logic step; interpolate positions with lerp(prev, cur, alpha) for smooth motion.
    virtual void render(Renderer& renderer, double alpha) {
        (void)renderer;
        (void)alpha;
    }
    // Overlay scenes let the scene below render first.
    virtual bool is_overlay() const { return false; }
    // Window close / Alt+F4 while this scene is on top. Return false to refuse (e.g. to ask
    // "Save changes?" first) and call app().quit() later.
    virtual bool on_quit_request() { return true; }

    // The App that owns this scene (valid from on_enter()).
    App& app() const { return *app_; }

private:
    friend class App;
    App* app_ = nullptr;
};

}  // namespace vs
