#pragma once
// Easing curves for the intro and menu animations (t in [0, 1]).

#include <algorithm>
#include <cmath>

namespace piton_game::ease {

inline float clamp01(float t) { return std::clamp(t, 0.0f, 1.0f); }

// Progress of `time` through the interval [start, start + length], clamped to [0, 1].
inline float phase(double time, double start, double length) {
    return clamp01(static_cast<float>((time - start) / length));
}

inline float out_cubic(float t) {
    t = clamp01(t);
    const float u = 1.0f - t;
    return 1.0f - u * u * u;
}

inline float in_out_sine(float t) { return 0.5f - 0.5f * std::cos(clamp01(t) * 3.14159265f); }

inline float out_back(float t) {
    t = clamp01(t);
    const float c1 = 1.70158f;
    const float c3 = c1 + 1.0f;
    const float u = t - 1.0f;
    return 1.0f + c3 * u * u * u + c1 * u * u;
}

inline float out_bounce(float t) {
    t = clamp01(t);
    const float n1 = 7.5625f;
    const float d1 = 2.75f;
    if (t < 1.0f / d1) return n1 * t * t;
    if (t < 2.0f / d1) {
        t -= 1.5f / d1;
        return n1 * t * t + 0.75f;
    }
    if (t < 2.5f / d1) {
        t -= 2.25f / d1;
        return n1 * t * t + 0.9375f;
    }
    t -= 2.625f / d1;
    return n1 * t * t + 0.984375f;
}

}  // namespace piton_game::ease
