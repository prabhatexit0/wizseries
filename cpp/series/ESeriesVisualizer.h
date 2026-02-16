// ─── WizSeries: Euler's Number (e) Series Visualizer ─────────────────────────
// Draws bars for each term 1/n! and overlays a running partial-sum line
// rapidly converging to e ≈ 2.71828.
// ─────────────────────────────────────────────────────────────────────────────
#pragma once

#include "ISeriesVisualizer.h"

#include <algorithm>
#include <cmath>
#include <vector>

class ESeriesVisualizer : public ISeriesVisualizer {
public:
    ESeriesVisualizer() { params_["terms"] = 12.0f; }

    void render(float time, float width, float /*height*/,
                GLRenderer& gl) override {
        const int terms =
            std::clamp(static_cast<int>(getParam("terms", 12.0f)), 1, 25);

        constexpr float mLeft   = 0.14f;
        constexpr float mRight  = 0.06f;
        constexpr float mBottom = 0.12f;
        constexpr float mTop    = 0.08f;

        const float xMin = -1.0f + mLeft;
        const float xMax =  1.0f - mRight;
        const float yMin = -1.0f + mBottom;
        const float yMax =  1.0f - mTop;

        constexpr float E_LIMIT = 2.71828182845f;

        // y-axis scaling: always show at least up to e
        const float yScale = E_LIMIT * 1.12f;

        const float barW   = (xMax - xMin) / static_cast<float>(terms);
        const float barGap = barW * 0.12f;

        // Animate: reveal ~4 terms per second (slower — fewer terms)
        const float revealed = time * 4.0f;
        const int   visible  = std::min(terms,
                                        static_cast<int>(revealed) + 1);

        // ── Horizontal gridlines ────────────────────────────────────────
        std::vector<Vertex> grid;
        {
            float step = 0.5f;
            for (float v = step; v < yScale; v += step) {
                float gy = yMin + (v / yScale) * (yMax - yMin);
                grid.push_back({xMin, gy, 0.78f, 0.76f, 0.74f, 0.25f});
                grid.push_back({xMax, gy, 0.78f, 0.76f, 0.74f, 0.25f});
            }
        }

        std::vector<Vertex> quads;
        quads.reserve(static_cast<size_t>(visible * 6));
        std::vector<Vertex> sumLine;
        sumLine.reserve(static_cast<size_t>(visible));

        float partialSum = 0.0f;
        float factorial = 1.0f;

        for (int n = 0; n < visible; ++n) {
            if (n > 0) factorial *= static_cast<float>(n);
            const float term = 1.0f / factorial;
            partialSum += term;

            const float alpha =
                std::clamp(revealed - static_cast<float>(n), 0.0f, 1.0f);

            const float x1 = xMin + static_cast<float>(n)     * barW + barGap;
            const float x2 = xMin + static_cast<float>(n + 1) * barW - barGap;
            const float by = yMin + (term / yScale) * (yMax - yMin);

            // Golden amber gradient
            float cr{}, cg{}, cb{};
            float hue = 0.12f - 0.06f * static_cast<float>(n)
                                      / static_cast<float>(std::max(terms - 1, 1));
            hsvToRgb(hue, 0.70f, 0.75f, cr, cg, cb);

            addQuad(quads, x1, yMin, x2, by, cr, cg, cb, alpha * 0.85f);

            // Partial-sum polyline (deep blue)
            const float sx = xMin + (static_cast<float>(n) + 0.5f) * barW;
            const float sy = yMin + (partialSum / yScale) * (yMax - yMin);
            sumLine.push_back({sx, sy, 0.10f, 0.25f, 0.65f, alpha});
        }

        // ── Axes ────────────────────────────────────────────────────────
        std::vector<Vertex> axes;
        axes.push_back({xMin, yMin, 0.30f, 0.28f, 0.26f, 0.8f});
        axes.push_back({xMax, yMin, 0.30f, 0.28f, 0.26f, 0.8f});
        axes.push_back({xMin, yMin, 0.30f, 0.28f, 0.26f, 0.8f});
        axes.push_back({xMin, yMax, 0.30f, 0.28f, 0.26f, 0.8f});

        // Y-axis tick marks
        {
            float step = 0.5f;
            for (float v = step; v < yScale; v += step) {
                float ty = yMin + (v / yScale) * (yMax - yMin);
                axes.push_back({xMin - 0.015f, ty, 0.30f, 0.28f, 0.26f, 0.7f});
                axes.push_back({xMin + 0.01f,  ty, 0.30f, 0.28f, 0.26f, 0.7f});
            }
        }

        // Convergence limit line at e
        if (visible >= terms) {
            const float limitY = yMin + (E_LIMIT / yScale) * (yMax - yMin);
            const float pulse  = 0.5f + 0.5f * std::sin(time * 3.0f);
            axes.push_back({xMin, limitY,
                            0.15f, 0.60f, 0.15f, 0.4f + 0.4f * pulse});
            axes.push_back({xMax, limitY,
                            0.15f, 0.60f, 0.15f, 0.4f + 0.4f * pulse});
        }

        gl.drawLines(grid);
        gl.drawTriangles(quads);
        gl.drawLines(axes);
        if (sumLine.size() >= 2) gl.drawLineStrip(sumLine);
    }
};
