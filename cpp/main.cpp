// ─── React + C++20 WASM Engine ──────────────────────────────────────────────
// Compiled with Emscripten → ES6 module, consumed by Vite/React.
// Exports:
//   - computePrimes(limit)  : compute-intensive prime sieve (C++20 ranges)
//   - initWebGL(canvasId)   : initialise a WebGL 2 context on a <canvas>
//   - renderFrame(r, g, b)  : clear the canvas with the given colour
// ────────────────────────────────────────────────────────────────────────────

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <numeric>
#include <ranges>
#include <span>
#include <string>
#include <vector>

#include <emscripten.h>
#include <emscripten/bind.h>
#include <emscripten/html5.h>
#include <GLES3/gl3.h>

// ─── Compute: Segmented Sieve of Eratosthenes (C++20) ──────────────────────
// Returns all primes up to `limit` using modern C++20 features.

namespace detail {

auto sieve_primes(std::uint32_t limit) -> std::vector<std::uint32_t> {
    if (limit < 2) return {};

    // Classic boolean sieve
    std::vector<bool> is_prime(limit + 1, true);
    is_prime[0] = is_prime[1] = false;

    for (std::uint32_t i = 2; i * i <= limit; ++i) {
        if (is_prime[i]) {
            for (std::uint32_t j = i * i; j <= limit; j += i) {
                is_prime[j] = false;
            }
        }
    }

    // C++20: use std::views to filter and collect primes
    auto indices = std::views::iota(std::uint32_t{0}, limit + 1);
    auto primes_view = indices | std::views::filter([&](std::uint32_t n) {
        return is_prime[n];
    });

    std::vector<std::uint32_t> result;
    result.reserve(static_cast<std::size_t>(
        static_cast<double>(limit) / std::log(static_cast<double>(limit) + 1) * 1.3
    ));
    std::ranges::copy(primes_view, std::back_inserter(result));

    return result;
}

} // namespace detail

// Exposed to JS: returns a string summary (count + last few primes).
auto computePrimes(std::uint32_t limit) -> std::string {
    auto primes = detail::sieve_primes(limit);

    if (primes.empty()) return "No primes found.";

    std::string result = "Found " + std::to_string(primes.size()) + " primes up to "
                         + std::to_string(limit) + ".\n";

    // Show the last 10 primes as a preview
    auto tail = std::span{primes};
    if (tail.size() > 10) {
        tail = tail.last(10);
        result += "... ";
    }
    result += "Last primes: ";
    for (std::size_t i = 0; i < tail.size(); ++i) {
        if (i > 0) result += ", ";
        result += std::to_string(tail[i]);
    }

    return result;
}

// ─── WebGL 2 Initialisation ────────────────────────────────────────────────

static EMSCRIPTEN_WEBGL_CONTEXT_HANDLE gl_context = 0;

auto initWebGL(const std::string& canvas_id) -> bool {
    // Build the selector: Emscripten expects "#canvasId"
    std::string selector = "#" + canvas_id;

    EmscriptenWebGLContextAttributes attrs;
    emscripten_webgl_init_context_attributes(&attrs);
    attrs.majorVersion = 2;           // WebGL 2
    attrs.minorVersion = 0;
    attrs.alpha        = true;
    attrs.depth        = true;
    attrs.antialias    = true;

    gl_context = emscripten_webgl_create_context(selector.c_str(), &attrs);
    if (gl_context <= 0) {
        return false;
    }

    emscripten_webgl_make_context_current(gl_context);

    // Initial clear to a dark blue-grey
    glClearColor(0.09f, 0.09f, 0.18f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    return true;
}

// Simple render pass: clear the canvas with the given RGB colour [0.0 – 1.0].
void renderFrame(float r, float g, float b) {
    if (gl_context <= 0) return;
    emscripten_webgl_make_context_current(gl_context);
    glClearColor(r, g, b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

// ─── Embind exports ────────────────────────────────────────────────────────

EMSCRIPTEN_BINDINGS(engine) {
    emscripten::function("computePrimes", &computePrimes);
    emscripten::function("initWebGL",     &initWebGL);
    emscripten::function("renderFrame",   &renderFrame);
}
