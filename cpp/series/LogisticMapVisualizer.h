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

        // Extra left/bottom margins for axis labels
        constexpr float mLeft   = 0.14f;
        constexpr float mRight  = 0.06f;
        constexpr float mBottom = 0.12f;
        constexpr float mTop    = 0.08f;

        const float xMin = -1.0f + mLeft;
        const float xMax =  1.0f - mRight;
        const float yMin = -1.0f + mBottom;
        const float yMax =  1.0f - mTop;

        // Number of columns scales with canvas pixel width
        int cols = std::clamp(static_cast<int>(width * 0.7f), 200, 1400);

        constexpr int warmup  = 300;   // transient iterations to discard
        constexpr int plotItr = 120;   // attractor samples per column

        // Animated left-to-right sweep (completes in ~2 s)
        const float revealFrac = std::clamp(time * 0.5f, 0.0f, 1.0f);
        const int   visCols    = std::max(1, static_cast<int>(
                                     static_cast<float>(cols) * revealFrac));

        // ── Gridlines ─────────────────────────────────────────────────────
        std::vector<Vertex> grid;
        // Horizontal gridlines at x = 0.25, 0.50, 0.75
        for (float v : {0.25f, 0.50f, 0.75f}) {
            float gy = yMin + (yMax - yMin) * v;
            grid.push_back({xMin, gy, 0.78f, 0.76f, 0.74f, 0.22f});
            grid.push_back({xMax, gy, 0.78f, 0.76f, 0.74f, 0.22f});
        }
        // Vertical gridlines at nice r values
        for (float rv : {1.5f, 2.0f, 2.5f, 3.0f, 3.5f, 4.0f}) {
            if (rv < rMin || rv > rMax) continue;
            float t = (rv - rMin) / (rMax - rMin);
            float gx = xMin + (xMax - xMin) * t;
            grid.push_back({gx, yMin, 0.78f, 0.76f, 0.74f, 0.22f});
            grid.push_back({gx, yMax, 0.78f, 0.76f, 0.74f, 0.22f});
        }

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

                // Deep blue → purple palette for light background
                float cr{}, cg{}, cb{};
                float hue = 0.65f + 0.15f * t;
                hsvToRgb(hue, 0.75f, 0.55f, cr, cg, cb);

                points.push_back({clipX, clipY, cr, cg, cb, 0.60f});
            }
        }

        // ── Axes (dark for light background) ──────────────────────────────
        std::vector<Vertex> axes;
        axes.push_back({xMin, yMin, 0.30f, 0.28f, 0.26f, 0.8f});
        axes.push_back({xMax, yMin, 0.30f, 0.28f, 0.26f, 0.8f});
        axes.push_back({xMin, yMin, 0.30f, 0.28f, 0.26f, 0.8f});
        axes.push_back({xMin, yMax, 0.30f, 0.28f, 0.26f, 0.8f});

        // X-axis (r) tick marks
        for (float rv : {1.5f, 2.0f, 2.5f, 3.0f, 3.5f, 4.0f}) {
            if (rv < rMin || rv > rMax) continue;
            float t = (rv - rMin) / (rMax - rMin);
            float tx = xMin + (xMax - xMin) * t;
            axes.push_back({tx, yMin - 0.015f, 0.30f, 0.28f, 0.26f, 0.7f});
            axes.push_back({tx, yMin + 0.01f,  0.30f, 0.28f, 0.26f, 0.7f});
        }

        // Y-axis (x) tick marks at 0.25, 0.50, 0.75
        for (float v : {0.25f, 0.50f, 0.75f}) {
            float ty = yMin + (yMax - yMin) * v;
            axes.push_back({xMin - 0.015f, ty, 0.30f, 0.28f, 0.26f, 0.7f});
            axes.push_back({xMin + 0.01f,  ty, 0.30f, 0.28f, 0.26f, 0.7f});
        }

        // Onset-of-chaos marker at r ≈ 3.57
        if (rMax > 3.57f) {
            const float chaosT = (3.57f - rMin) / (rMax - rMin);
            const float cx     = xMin + (xMax - xMin) * chaosT;
            axes.push_back({cx, yMin, 0.85f, 0.15f, 0.15f, 0.55f});
            axes.push_back({cx, yMax, 0.85f, 0.15f, 0.15f, 0.55f});
        }

        gl.drawLines(grid);
        gl.drawLines(axes);
        gl.drawPoints(points, 1.5f);
    }
};
