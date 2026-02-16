// ─── WizSeries: Apéry's Constant Visualizer ─────────────────────────────────
// Draws bars for each term 1/n³ and overlays a running partial-sum line
// converging to ζ(3) ≈ 1.20206.
// ─────────────────────────────────────────────────────────────────────────────
#pragma once

#include "ISeriesVisualizer.h"

#include <algorithm>
#include <cmath>
#include <vector>

class AperyConstantVisualizer : public ISeriesVisualizer {
public:
    AperyConstantVisualizer() { params_["terms"] = 30.0f; }

    void render(float time, float width, float /*height*/,
                GLRenderer& gl) override {
        const int terms =
            std::clamp(static_cast<int>(getParam("terms", 30.0f)), 1, 200);

        constexpr float mLeft   = 0.14f;
        constexpr float mRight  = 0.06f;
        constexpr float mBottom = 0.12f;
        constexpr float mTop    = 0.08f;

        const float xMin = -1.0f + mLeft;
        const float xMax =  1.0f - mRight;
        const float yMin = -1.0f + mBottom;
        const float yMax =  1.0f - mTop;

        constexpr float LIMIT = 1.20205690f; // ζ(3)

        // y-axis scaling: always show at least up to the limit
        const float yScale = LIMIT * 1.15f;

        const float barW   = (xMax - xMin) / static_cast<float>(terms);
        const float barGap = barW * 0.12f;

        // Animate: reveal ~10 terms per second
        const float revealed = time * 10.0f;
        const int   visible  = std::min(terms,
                                        static_cast<int>(revealed) + 1);

        // ── Horizontal gridlines ────────────────────────────────────────
        std::vector<Vertex> grid;
        {
            float step = 0.25f;
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

        for (int n = 1; n <= visible; ++n) {
            const float nf = static_cast<float>(n);
            const float term = 1.0f / (nf * nf * nf);
            partialSum += term;

            const float alpha =
                std::clamp(revealed - static_cast<float>(n - 1), 0.0f, 1.0f);

            const float x1 = xMin + static_cast<float>(n - 1) * barW + barGap;
            const float x2 = xMin + static_cast<float>(n)     * barW - barGap;
            const float by = yMin + (term / yScale) * (yMax - yMin);

            // Rose-magenta gradient
            float cr{}, cg{}, cb{};
            float hue = 0.90f - 0.06f * static_cast<float>(n - 1)
                                      / static_cast<float>(std::max(terms - 1, 1));
            hsvToRgb(hue, 0.60f, 0.70f, cr, cg, cb);

            addQuad(quads, x1, yMin, x2, by, cr, cg, cb, alpha * 0.85f);

            // Partial-sum polyline (deep teal)
            const float sx = xMin + (static_cast<float>(n) - 0.5f) * barW;
            const float sy = yMin + (partialSum / yScale) * (yMax - yMin);
            sumLine.push_back({sx, sy, 0.10f, 0.45f, 0.50f, alpha});
        }

        // ── Axes ────────────────────────────────────────────────────────
        std::vector<Vertex> axes;
        axes.push_back({xMin, yMin, 0.30f, 0.28f, 0.26f, 0.8f});
        axes.push_back({xMax, yMin, 0.30f, 0.28f, 0.26f, 0.8f});
        axes.push_back({xMin, yMin, 0.30f, 0.28f, 0.26f, 0.8f});
        axes.push_back({xMin, yMax, 0.30f, 0.28f, 0.26f, 0.8f});

        // Y-axis tick marks
        {
            float step = 0.25f;
            for (float v = step; v < yScale; v += step) {
                float ty = yMin + (v / yScale) * (yMax - yMin);
                axes.push_back({xMin - 0.015f, ty, 0.30f, 0.28f, 0.26f, 0.7f});
                axes.push_back({xMin + 0.01f,  ty, 0.30f, 0.28f, 0.26f, 0.7f});
            }
        }

        // Convergence limit line at ζ(3)
        if (visible >= terms) {
            const float limitY = yMin + (LIMIT / yScale) * (yMax - yMin);
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
