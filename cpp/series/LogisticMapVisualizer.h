// ─── WizSeries: Logistic Map / Bifurcation Diagram ──────────────────────────
// Iterates  xₙ₊₁ = r·xₙ·(1 − xₙ)  for a sweep of growth-rate values r and
// plots the resulting attractor as a cloud of coloured points — the classic
// bifurcation diagram from chaos theory.
// ─────────────────────────────────────────────────────────────────────────────
#pragma once

#include "ISeriesVisualizer.h"

#include <algorithm>
#include <cmath>
#include <vector>

class LogisticMapVisualizer : public ISeriesVisualizer {
public:
    LogisticMapVisualizer() { params_["growth_rate"] = 4.0f; }

    void render(float time, float width, float height,
                GLRenderer& gl) override {
        const float rMax =
            std::clamp(getParam("growth_rate", 4.0f), 1.0f, 4.0f);
        constexpr float rMin = 1.0f;

        constexpr float margin = 0.08f;
        constexpr float xMin   = -1.0f + margin;
        constexpr float xMax   =  1.0f - margin;
        constexpr float yMin   = -1.0f + margin;
        constexpr float yMax   =  1.0f - margin;

        // Number of columns scales with canvas pixel width
        int cols = std::clamp(static_cast<int>(width * 0.7f), 200, 1400);

        constexpr int warmup  = 300;   // transient iterations to discard
        constexpr int plotItr = 120;   // attractor samples per column

        // Animated left-to-right sweep (completes in ~2 s)
        const float revealFrac = std::clamp(time * 0.5f, 0.0f, 1.0f);
        const int   visCols    = std::max(1, static_cast<int>(
                                     static_cast<float>(cols) * revealFrac));

        std::vector<Vertex> points;
        points.reserve(static_cast<size_t>(visCols) * plotItr);

        for (int col = 0; col < visCols; ++col) {
            const float t =
                static_cast<float>(col) / static_cast<float>(cols - 1);
            const float r    = rMin + (rMax - rMin) * t;
            const float clipX = xMin + (xMax - xMin) * t;

            // Iterate the map
            float x = 0.5f;
            for (int i = 0; i < warmup; ++i)
                x = r * x * (1.0f - x);

            for (int i = 0; i < plotItr; ++i) {
                x = r * x * (1.0f - x);
                const float clipY = yMin + (yMax - yMin) * x;

                // Colour: hue shifts from cyan to magenta across r
                float cr{}, cg{}, cb{};
                float hue = 0.50f + 0.30f * t;
                hsvToRgb(hue, 0.70f, 0.92f, cr, cg, cb);

                points.push_back({clipX, clipY, cr, cg, cb, 0.55f});
            }
        }

        // ── Axes ────────────────────────────────────────────────────────────
        std::vector<Vertex> axes;
        axes.push_back({xMin, yMin, 0.30f, 0.30f, 0.40f, 0.6f});
        axes.push_back({xMax, yMin, 0.30f, 0.30f, 0.40f, 0.6f});
        axes.push_back({xMin, yMin, 0.30f, 0.30f, 0.40f, 0.6f});
        axes.push_back({xMin, yMax, 0.30f, 0.30f, 0.40f, 0.6f});

        // Onset-of-chaos marker at r ≈ 3.57
        if (rMax > 3.57f) {
            const float chaosT = (3.57f - rMin) / (rMax - rMin);
            const float cx     = xMin + (xMax - xMin) * chaosT;
            axes.push_back({cx, yMin, 0.9f, 0.3f, 0.3f, 0.4f});
            axes.push_back({cx, yMax, 0.9f, 0.3f, 0.3f, 0.4f});
        }

        gl.drawLines(axes);
        gl.drawPoints(points, 1.5f);
    }
};
