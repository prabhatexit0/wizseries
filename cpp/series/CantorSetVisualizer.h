// ─── WizSeries: Cantor Set Visualizer ───────────────────────────────────────
// Renders the recursive middle-thirds removal that produces the Cantor set.
// Each level is drawn as a row of coloured bars; deeper levels fade in over
// time to animate the infinite descent.
// ─────────────────────────────────────────────────────────────────────────────
#pragma once

#include "ISeriesVisualizer.h"

#include <algorithm>
#include <cmath>
#include <vector>

class CantorSetVisualizer : public ISeriesVisualizer {
public:
    CantorSetVisualizer() { params_["depth"] = 6.0f; }

    void render(float time, float width, float height,
                GLRenderer& gl) override {
        const int depth =
            std::clamp(static_cast<int>(getParam("depth", 6.0f)), 1, 12);

        // Clip-space margins — extra left/bottom for axis labels
        constexpr float mLeft   = 0.14f;
        constexpr float mRight  = 0.06f;
        constexpr float mBottom = 0.10f;
        constexpr float mTop    = 0.08f;

        const float xMin = -1.0f + mLeft;
        const float xMax =  1.0f - mRight;
        const float yMin = -1.0f + mBottom;
        const float yMax =  1.0f - mTop;

        const float totalH = yMax - yMin;
        const float gap    = totalH / static_cast<float>(depth + 1);
        const float barH   = gap * 0.70f;

        // Progressive reveal: ~1.5 levels per second
        const float revealed = time * 1.5f;

        std::vector<Vertex> quads;
        quads.reserve(static_cast<size_t>(6 * ((1 << (depth + 1)) - 1)));

        generateCantor(quads, 0.0f, 1.0f, 0, depth,
                       xMin, xMax, yMax, barH, gap, revealed);

        // ── Gridlines (subtle horizontal guides per level) ────────────────
        std::vector<Vertex> grid;
        for (int lv = 0; lv <= depth; ++lv) {
            float y = yMax - static_cast<float>(lv) * gap - barH * 0.5f;
            grid.push_back({xMin, y, 0.78f, 0.76f, 0.74f, 0.25f});
            grid.push_back({xMax, y, 0.78f, 0.76f, 0.74f, 0.25f});
        }

        // ── Axes (dark grey for light background) ─────────────────────────
        std::vector<Vertex> axes;
        axes.push_back({xMin, yMin, 0.30f, 0.28f, 0.26f, 0.8f});
        axes.push_back({xMax, yMin, 0.30f, 0.28f, 0.26f, 0.8f});
        axes.push_back({xMin, yMin, 0.30f, 0.28f, 0.26f, 0.8f});
        axes.push_back({xMin, yMax, 0.30f, 0.28f, 0.26f, 0.8f});

        // Level separator tick marks on the left axis
        for (int lv = 0; lv <= depth; ++lv) {
            float y = yMax - static_cast<float>(lv) * gap - barH * 0.5f;
            axes.push_back({xMin - 0.015f, y, 0.30f, 0.28f, 0.26f, 0.7f});
            axes.push_back({xMin + 0.01f,  y, 0.30f, 0.28f, 0.26f, 0.7f});
        }

        gl.drawLines(grid);
        gl.drawTriangles(quads);
        gl.drawLines(axes);
    }

private:
    void generateCantor(std::vector<Vertex>& quads,
                        float left, float right,
                        int level, int maxDepth,
                        float xMin, float xMax,
                        float yTop, float barH, float gap,
                        float revealed) {
        if (level > maxDepth) return;

        const float alpha = std::clamp(revealed - static_cast<float>(level),
                                       0.0f, 1.0f);
        if (alpha <= 0.0f) return;

        // Map [0,1] segment → clip-space x
        const float x1 = xMin + left  * (xMax - xMin);
        const float x2 = xMin + right * (xMax - xMin);
        const float y1 = yTop - static_cast<float>(level) * gap;
        const float y2 = y1 - barH;

        // Rich indigo → violet → magenta for light background
        float cr{}, cg{}, cb{};
        float hue = 0.72f - static_cast<float>(level) * 0.04f;
        float sat = 0.70f + 0.10f * alpha;
        float val = 0.50f + 0.20f * alpha;
        hsvToRgb(hue, sat, val, cr, cg, cb);

        addQuad(quads, x1, y2, x2, y1, cr, cg, cb, alpha * 0.92f);

        // Recurse: keep first and last thirds, remove the middle
        const float third = (right - left) / 3.0f;
        generateCantor(quads, left,          left + third,
                       level + 1, maxDepth,
                       xMin, xMax, yTop, barH, gap, revealed);
        generateCantor(quads, right - third, right,
                       level + 1, maxDepth,
                       xMin, xMax, yTop, barH, gap, revealed);
    }
};
