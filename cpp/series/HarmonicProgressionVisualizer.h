// ─── WizSeries: Harmonic Progression Visualizer ─────────────────────────────
// Draws bars for each term 1/k of the harmonic series and overlays a running
// partial-sum line to illustrate the slow (logarithmic) divergence.
// ─────────────────────────────────────────────────────────────────────────────
#pragma once

#include "ISeriesVisualizer.h"

#include <algorithm>
#include <cmath>
#include <vector>

class HarmonicProgressionVisualizer : public ISeriesVisualizer {
public:
    HarmonicProgressionVisualizer() { params_["terms"] = 30.0f; }

    void render(float time, float width, float height,
                GLRenderer& gl) override {
        const int terms =
            std::clamp(static_cast<int>(getParam("terms", 30.0f)), 1, 2000);

        // Extra left/bottom margins for axis labels
        constexpr float mLeft   = 0.14f;
        constexpr float mRight  = 0.06f;
        constexpr float mBottom = 0.12f;
        constexpr float mTop    = 0.08f;

        const float xMin = -1.0f + mLeft;
        const float xMax =  1.0f - mRight;
        const float yMin = -1.0f + mBottom;
        const float yMax =  1.0f - mTop;

        // Pre-compute max partial sum for y-axis scaling
        float maxSum = 0.0f;
        for (int k = 1; k <= terms; ++k) maxSum += 1.0f / static_cast<float>(k);
        const float yScale = std::max(1.0f, maxSum) * 1.1f;

        const float barW   = (xMax - xMin) / static_cast<float>(terms);
        const float barGap = barW * 0.12f;

        // Animate: reveal ~10 terms per second
        const float revealed = time * 10.0f;
        const int   visible  = std::min(terms,
                                        static_cast<int>(revealed) + 1);

        // ── Horizontal gridlines ──────────────────────────────────────────
        std::vector<Vertex> grid;
        {
            // Choose nice grid spacing
            float step = 1.0f;
            if (yScale > 8.0f) step = 2.0f;
            if (yScale > 16.0f) step = 4.0f;

            for (float v = step; v < yScale; v += step) {
                float gy = yMin + (v / yScale) * (yMax - yMin);
                grid.push_back({xMin, gy, 0.78f, 0.76f, 0.74f, 0.30f});
                grid.push_back({xMax, gy, 0.78f, 0.76f, 0.74f, 0.30f});
            }
        }

        std::vector<Vertex> quads;
        quads.reserve(static_cast<size_t>(visible * 6));
        std::vector<Vertex> sumLine;
        sumLine.reserve(static_cast<size_t>(visible));

        float partialSum = 0.0f;

        for (int k = 1; k <= visible; ++k) {
            const float term  = 1.0f / static_cast<float>(k);
            partialSum += term;

            const float alpha =
                std::clamp(revealed - static_cast<float>(k - 1), 0.0f, 1.0f);

            // Bar geometry
            const float x1 = xMin + static_cast<float>(k - 1) * barW + barGap;
            const float x2 = xMin + static_cast<float>(k)     * barW - barGap;
            const float by = yMin + (term / yScale) * (yMax - yMin);

            // Warm terracotta gradient for light theme
            float cr{}, cg{}, cb{};
            float hue = 0.07f - 0.05f * static_cast<float>(k - 1)
                                      / static_cast<float>(std::max(terms - 1, 1));
            hsvToRgb(hue, 0.65f, 0.80f, cr, cg, cb);

            addQuad(quads, x1, yMin, x2, by, cr, cg, cb, alpha * 0.85f);

            // Partial-sum polyline (deep blue)
            const float sx = xMin + (static_cast<float>(k) - 0.5f) * barW;
            const float sy = yMin + (partialSum / yScale) * (yMax - yMin);
            sumLine.push_back({sx, sy, 0.10f, 0.30f, 0.70f, alpha});
        }

        // ── Axes (dark for light background) ──────────────────────────────
        std::vector<Vertex> axes;
        axes.push_back({xMin, yMin, 0.30f, 0.28f, 0.26f, 0.8f});
        axes.push_back({xMax, yMin, 0.30f, 0.28f, 0.26f, 0.8f});
        axes.push_back({xMin, yMin, 0.30f, 0.28f, 0.26f, 0.8f});
        axes.push_back({xMin, yMax, 0.30f, 0.28f, 0.26f, 0.8f});

        // Y-axis tick marks
        {
            float step = 1.0f;
            if (yScale > 8.0f) step = 2.0f;
            if (yScale > 16.0f) step = 4.0f;
            for (float v = step; v < yScale; v += step) {
                float ty = yMin + (v / yScale) * (yMax - yMin);
                axes.push_back({xMin - 0.015f, ty, 0.30f, 0.28f, 0.26f, 0.7f});
                axes.push_back({xMin + 0.01f,  ty, 0.30f, 0.28f, 0.26f, 0.7f});
            }
        }

        // Pulsing divergence indicator at current sum level
        if (visible >= terms && terms > 5) {
            const float sumY  = yMin + (partialSum / yScale) * (yMax - yMin);
            const float pulse = 0.5f + 0.5f * std::sin(time * 3.0f);
            axes.push_back({xMin, sumY, 0.85f, 0.20f, 0.20f, 0.4f + 0.4f * pulse});
            axes.push_back({xMax, sumY, 0.85f, 0.20f, 0.20f, 0.4f + 0.4f * pulse});
        }

        gl.drawLines(grid);
        gl.drawTriangles(quads);
        gl.drawLines(axes);
        if (sumLine.size() >= 2) gl.drawLineStrip(sumLine);
    }
};
