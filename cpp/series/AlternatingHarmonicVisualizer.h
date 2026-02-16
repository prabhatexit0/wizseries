// ─── WizSeries: Alternating Harmonic Series Visualizer ───────────────────────
// Draws bars for each term (-1)^(n+1)/n of the alternating harmonic series
// and overlays a running partial-sum line oscillating toward ln(2) ≈ 0.6931.
// ─────────────────────────────────────────────────────────────────────────────
#pragma once

#include "ISeriesVisualizer.h"

#include <algorithm>
#include <cmath>
#include <vector>

class AlternatingHarmonicVisualizer : public ISeriesVisualizer {
public:
    AlternatingHarmonicVisualizer() { params_["terms"] = 30.0f; }

    void render(float time, float width, float /*height*/,
                GLRenderer& gl) override {
        const int terms =
            std::clamp(static_cast<int>(getParam("terms", 30.0f)), 1, 2000);

        constexpr float mLeft   = 0.14f;
        constexpr float mRight  = 0.06f;
        constexpr float mBottom = 0.12f;
        constexpr float mTop    = 0.08f;

        const float xMin = -1.0f + mLeft;
        const float xMax =  1.0f - mRight;
        const float yMid =  0.0f;
        const float yExt =  1.0f - std::max(mTop, mBottom);

        constexpr float LIMIT = 0.69314718f; // ln(2)

        // Pre-scan for scaling
        float maxAbsVal = 0.0f;
        float maxAbsSum = 0.0f;
        {
            float s = 0.0f;
            for (int n = 1; n <= terms; ++n) {
                float sign = (n % 2 == 1) ? 1.0f : -1.0f;
                float term = sign / static_cast<float>(n);
                maxAbsVal = std::max(maxAbsVal, std::abs(term));
                s += term;
                maxAbsSum = std::max(maxAbsSum, std::abs(s));
            }
        }
        const float scale = std::max({maxAbsVal, maxAbsSum, 0.001f});

        const float barW   = (xMax - xMin) / static_cast<float>(terms);
        const float barGap = barW * 0.10f;

        const float revealed = time * 8.0f;
        const int   visible  = std::min(terms,
                                        static_cast<int>(revealed) + 1);

        // ── Horizontal gridlines ────────────────────────────────────────
        std::vector<Vertex> grid;
        {
            float step = scale / 4.0f;
            if (step < 0.01f) step = 0.01f;
            float mag = std::pow(10.0f, std::floor(std::log10(step)));
            step = std::ceil(step / mag) * mag;

            for (float v = step; v < scale; v += step) {
                float gy = yMid + (v / scale) * yExt;
                float gyn = yMid - (v / scale) * yExt;
                grid.push_back({xMin, gy,  0.78f, 0.76f, 0.74f, 0.25f});
                grid.push_back({xMax, gy,  0.78f, 0.76f, 0.74f, 0.25f});
                grid.push_back({xMin, gyn, 0.78f, 0.76f, 0.74f, 0.25f});
                grid.push_back({xMax, gyn, 0.78f, 0.76f, 0.74f, 0.25f});
            }
        }

        std::vector<Vertex> quads;
        quads.reserve(static_cast<size_t>(visible * 6));
        std::vector<Vertex> sumLine;
        sumLine.reserve(static_cast<size_t>(visible));

        float partialSum = 0.0f;

        for (int n = 1; n <= visible; ++n) {
            float sign = (n % 2 == 1) ? 1.0f : -1.0f;
            float term = sign / static_cast<float>(n);
            partialSum += term;

            const float alpha =
                std::clamp(revealed - static_cast<float>(n - 1), 0.0f, 1.0f);

            const float x1 = xMin + static_cast<float>(n - 1) * barW + barGap;
            const float x2 = xMin + static_cast<float>(n)     * barW - barGap;
            const float bh = (term / scale) * yExt;

            // Teal for positive, coral for negative
            float cr{}, cg{}, cb{};
            if (term >= 0.0f)
                hsvToRgb(0.52f, 0.65f, 0.65f, cr, cg, cb);
            else
                hsvToRgb(0.02f, 0.65f, 0.70f, cr, cg, cb);

            float y1 = yMid;
            float y2 = yMid + bh;
            if (y1 > y2) std::swap(y1, y2);

            addQuad(quads, x1, y1, x2, y2, cr, cg, cb, alpha * 0.85f);

            // Running sum polyline (deep amber)
            const float sx = xMin + (static_cast<float>(n) - 0.5f) * barW;
            const float sy = yMid + (partialSum / scale) * yExt;
            sumLine.push_back({sx, sy, 0.80f, 0.50f, 0.05f, alpha});
        }

        // ── Axes ────────────────────────────────────────────────────────
        std::vector<Vertex> axes;
        // Horizontal zero-line
        axes.push_back({xMin, yMid, 0.30f, 0.28f, 0.26f, 0.8f});
        axes.push_back({xMax, yMid, 0.30f, 0.28f, 0.26f, 0.8f});
        // Left vertical axis
        axes.push_back({xMin, yMid - yExt, 0.30f, 0.28f, 0.26f, 0.8f});
        axes.push_back({xMin, yMid + yExt, 0.30f, 0.28f, 0.26f, 0.8f});

        // Convergence limit line at ln(2)
        if (visible >= terms) {
            const float limitY = yMid + (LIMIT / scale) * yExt;
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
