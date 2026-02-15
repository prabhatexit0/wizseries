// ─── WizSeries: Geometric Progression Visualizer ────────────────────────────
// Draws bars for successive powers  a·rᵏ  (a = 1) and overlays the partial-
// sum line.  Convergence (|r| < 1) or divergence (|r| ≥ 1) is immediately
// visible; negative ratios produce alternating-sign bars.
// ─────────────────────────────────────────────────────────────────────────────
#pragma once

#include "ISeriesVisualizer.h"

#include <algorithm>
#include <cmath>
#include <vector>

class GeometricProgressionVisualizer : public ISeriesVisualizer {
public:
    GeometricProgressionVisualizer() {
        params_["ratio"] = 0.70f;
        params_["terms"] = 15.0f;
    }

    void render(float time, float width, float height,
                GLRenderer& gl) override {
        const float ratio =
            std::clamp(getParam("ratio", 0.70f), -2.0f, 2.0f);
        const int terms =
            std::clamp(static_cast<int>(getParam("terms", 15.0f)), 1, 50);

        constexpr float margin = 0.08f;
        constexpr float xMin   = -1.0f + margin;
        constexpr float xMax   =  1.0f - margin;
        constexpr float yMid   =  0.0f;           // bars grow up/down from centre
        constexpr float yExt   =  1.0f - margin;  // max extent above/below centre

        // ── Pre-scan for scaling ────────────────────────────────────────────
        float maxAbsVal = 0.0f;
        float maxAbsSum = 0.0f;
        {
            float v = 1.0f, s = 0.0f;
            for (int k = 0; k < terms; ++k) {
                maxAbsVal = std::max(maxAbsVal, std::abs(v));
                s += v;
                maxAbsSum = std::max(maxAbsSum, std::abs(s));
                v *= ratio;
            }
        }
        const float scale = std::max({maxAbsVal, maxAbsSum, 0.001f});

        const float barW   = (xMax - xMin) / static_cast<float>(terms);
        const float barGap = barW * 0.10f;

        const float revealed = time * 8.0f;
        const int   visible  = std::min(terms,
                                        static_cast<int>(revealed) + 1);

        std::vector<Vertex> quads;
        quads.reserve(static_cast<size_t>(visible * 6));
        std::vector<Vertex> sumLine;
        sumLine.reserve(static_cast<size_t>(visible));

        float val = 1.0f;
        float partialSum = 0.0f;

        for (int k = 0; k < visible; ++k) {
            const float alpha =
                std::clamp(revealed - static_cast<float>(k), 0.0f, 1.0f);
            partialSum += val;

            const float x1 = xMin + static_cast<float>(k)     * barW + barGap;
            const float x2 = xMin + static_cast<float>(k + 1) * barW - barGap;
            const float bh = (val / scale) * yExt;

            float cr{}, cg{}, cb{};
            if (val >= 0.0f)
                hsvToRgb(0.38f, 0.75f, 0.85f, cr, cg, cb);   // green
            else
                hsvToRgb(0.00f, 0.75f, 0.85f, cr, cg, cb);   // red

            float y1 = yMid;
            float y2 = yMid + bh;
            if (y1 > y2) std::swap(y1, y2);

            addQuad(quads, x1, y1, x2, y2, cr, cg, cb, alpha);

            // Running sum polyline (gold)
            const float sx = xMin + (static_cast<float>(k) + 0.5f) * barW;
            const float sy = yMid + (partialSum / scale) * yExt;
            sumLine.push_back({sx, sy, 1.0f, 0.85f, 0.25f, alpha});

            val *= ratio;
        }

        // ── Axes ────────────────────────────────────────────────────────────
        std::vector<Vertex> axes;
        // Horizontal zero-line
        axes.push_back({xMin, yMid, 0.35f, 0.35f, 0.45f, 0.7f});
        axes.push_back({xMax, yMid, 0.35f, 0.35f, 0.45f, 0.7f});

        // Convergence limit line for |r| < 1
        if (std::abs(ratio) < 1.0f && std::abs(1.0f - ratio) > 1e-6f
            && visible >= terms) {
            const float limit  = 1.0f / (1.0f - ratio);
            const float limitY = yMid + (limit / scale) * yExt;
            const float pulse  = 0.5f + 0.5f * std::sin(time * 3.0f);
            axes.push_back({xMin, limitY,
                            0.30f, 1.0f, 0.40f, 0.35f + 0.35f * pulse});
            axes.push_back({xMax, limitY,
                            0.30f, 1.0f, 0.40f, 0.35f + 0.35f * pulse});
        }

        gl.drawTriangles(quads);
        gl.drawLines(axes);
        if (sumLine.size() >= 2) gl.drawLineStrip(sumLine);
    }
};
