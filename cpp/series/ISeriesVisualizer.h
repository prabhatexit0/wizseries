// ─── WizSeries: Abstract base for every series visualizer ───────────────────
#pragma once

#include "GLRenderer.h"

#include <cmath>
#include <string>
#include <unordered_map>

class ISeriesVisualizer {
public:
    virtual ~ISeriesVisualizer() = default;

    /// Called once per frame.  `time` is seconds since the visualizer became
    /// active; `width`/`height` are the canvas pixel dimensions.
    virtual void render(float time, float width, float height,
                        GLRenderer& gl) = 0;

    /// Set a named parameter (e.g. "depth", "ratio").
    virtual void setParam(const std::string& name, float value) {
        params_[name] = value;
    }

    /// Read back a parameter with an optional default.
    [[nodiscard]] float getParam(const std::string& name,
                                 float defaultVal = 0.0f) const {
        auto it = params_.find(name);
        return it != params_.end() ? it->second : defaultVal;
    }

protected:
    std::unordered_map<std::string, float> params_;

    // ── Colour helpers ──────────────────────────────────────────────────────

    /// HSV → RGB  (h, s, v all in [0, 1]).
    static void hsvToRgb(float h, float s, float v,
                         float& r, float& g, float& b) {
        h = h - static_cast<int>(h);
        if (h < 0.0f) h += 1.0f;
        int   i = static_cast<int>(h * 6.0f);
        float f = h * 6.0f - static_cast<float>(i);
        float p = v * (1.0f - s);
        float q = v * (1.0f - f * s);
        float t = v * (1.0f - (1.0f - f) * s);
        switch (i % 6) {
            case 0: r = v; g = t; b = p; break;
            case 1: r = q; g = v; b = p; break;
            case 2: r = p; g = v; b = t; break;
            case 3: r = p; g = q; b = v; break;
            case 4: r = t; g = p; b = v; break;
            case 5: r = v; g = p; b = q; break;
        }
    }
};
