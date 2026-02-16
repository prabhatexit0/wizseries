// ─── WizSeries: Central Manager ─────────────────────────────────────────────
// Owns the WebGL context, the shared GLRenderer, and all visualizer instances.
// Exposed to JavaScript via Emscripten embind.
// ─────────────────────────────────────────────────────────────────────────────
#pragma once

#include "ISeriesVisualizer.h"
#include "AlternatingHarmonicVisualizer.h"
#include "AperyConstantVisualizer.h"
#include "BaselProblemVisualizer.h"
#include "CantorSetVisualizer.h"
#include "ESeriesVisualizer.h"
#include "GeometricProgressionVisualizer.h"
#include "GregoryLeibnizVisualizer.h"
#include "HarmonicProgressionVisualizer.h"
#include "InverseGeometricVisualizer.h"
#include "LogisticMapVisualizer.h"

#include <emscripten.h>
#include <emscripten/html5.h>

#include <memory>
#include <string>
#include <unordered_map>

class SeriesManager {
public:
    SeriesManager() {
        visualizers_["cantor"]       = std::make_unique<CantorSetVisualizer>();
        visualizers_["harmonic"]     = std::make_unique<HarmonicProgressionVisualizer>();
        visualizers_["geometric"]    = std::make_unique<GeometricProgressionVisualizer>();
        visualizers_["logistic"]     = std::make_unique<LogisticMapVisualizer>();
        visualizers_["basel"]        = std::make_unique<BaselProblemVisualizer>();
        visualizers_["alt_harmonic"] = std::make_unique<AlternatingHarmonicVisualizer>();
        visualizers_["e_series"]     = std::make_unique<ESeriesVisualizer>();
        visualizers_["inv_geometric"]= std::make_unique<InverseGeometricVisualizer>();
        visualizers_["gregory_leibniz"] = std::make_unique<GregoryLeibnizVisualizer>();
        visualizers_["apery"]        = std::make_unique<AperyConstantVisualizer>();
        active_ = "cantor";
    }

    /// Create a WebGL 2 context on the given canvas and compile shaders.
    bool initGL(const std::string& canvasId) {
        const std::string selector = "#" + canvasId;

        EmscriptenWebGLContextAttributes attrs;
        emscripten_webgl_init_context_attributes(&attrs);
        attrs.majorVersion = 2;
        attrs.minorVersion = 0;
        attrs.alpha        = true;
        attrs.depth        = false;
        attrs.antialias    = true;

        ctx_ = emscripten_webgl_create_context(selector.c_str(), &attrs);
        if (ctx_ <= 0) return false;

        emscripten_webgl_make_context_current(ctx_);

        if (!renderer_.init()) return false;

        ready_ = true;
        return true;
    }

    /// Drive one frame of the active visualizer.
    void render(float time, float width, float height) {
        if (!ready_ || ctx_ <= 0) return;
        emscripten_webgl_make_context_current(ctx_);

        renderer_.beginFrame(width, height);

        auto it = visualizers_.find(active_);
        if (it != visualizers_.end()) {
            it->second->render(time, width, height, renderer_);
        }
    }

    /// Switch the active visualizer by key name.
    void setActiveVisualizer(const std::string& name) {
        if (visualizers_.count(name)) active_ = name;
    }

    [[nodiscard]] std::string getActiveVisualizer() const { return active_; }

    /// Forward a named parameter to the *active* visualizer.
    void setParam(const std::string& name, float value) {
        auto it = visualizers_.find(active_);
        if (it != visualizers_.end()) it->second->setParam(name, value);
    }

private:
    std::unordered_map<std::string, std::unique_ptr<ISeriesVisualizer>>
        visualizers_;
    std::string active_;
    GLRenderer  renderer_;
    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE ctx_ = 0;
    bool ready_ = false;
};
