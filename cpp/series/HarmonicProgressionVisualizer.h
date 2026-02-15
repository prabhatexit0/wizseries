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
            std::clamp(static_cast<int>(getParam("terms", 30.0f)), 1, 500);

        constexpr float margin = 0.08f;
        constexpr float xMin   = -1.0f + margin;
        constexpr float xMax   =  1.0f - margin;
        constexpr float yMin   = -1.0f + margin;
        constexpr float yMax   =  1.0f - margin * 3.0f;

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

            // Warm gradient: yellow → orange → red
            float cr{}, cg{}, cb{};
            float hue = 0.10f - 0.10f * static_cast<float>(k - 1)
                                      / static_cast<float>(std::max(terms - 1, 1));
            hsvToRgb(hue, 0.80f, 0.90f, cr, cg, cb);

            addQuad(quads, x1, yMin, x2, by, cr, cg, cb, alpha);

            // Partial-sum polyline (cyan)
            const float sx = xMin + (static_cast<float>(k) - 0.5f) * barW;
            const float sy = yMin + (partialSum / yScale) * (yMax - yMin);
            sumLine.push_back({sx, sy, 0.30f, 0.90f, 1.0f, alpha});
        }

        // ── Axes ────────────────────────────────────────────────────────────
        std::vector<Vertex> axes;
        axes.push_back({xMin, yMin, 0.30f, 0.30f, 0.40f, 0.7f});
        axes.push_back({xMax, yMin, 0.30f, 0.30f, 0.40f, 0.7f});
        axes.push_back({xMin, yMin, 0.30f, 0.30f, 0.40f, 0.7f});
        axes.push_back({xMin, yMax, 0.30f, 0.30f, 0.40f, 0.7f});

        // Pulsing divergence indicator at current sum level
        if (visible >= terms && terms > 5) {
            const float sumY  = yMin + (partialSum / yScale) * (yMax - yMin);
            const float pulse = 0.5f + 0.5f * std::sin(time * 3.0f);
            float gr{}, gg{}, gb{};
            hsvToRgb(0.55f, 0.85f, 0.4f + 0.5f * pulse, gr, gg, gb);
            axes.push_back({xMin, sumY, gr, gg, gb, 0.7f});
            axes.push_back({xMax, sumY, gr, gg, gb, 0.7f});
        }

        gl.drawTriangles(quads);
        gl.drawLines(axes);
        if (sumLine.size() >= 2) gl.drawLineStrip(sumLine);
    }
};
