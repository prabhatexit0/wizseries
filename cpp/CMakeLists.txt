cmake_minimum_required(VERSION 3.20)
project(ReactWasmEngine VERSION 1.0.0 LANGUAGES CXX)

# ─── C++20 Standard ──────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ─── Export compile_commands.json for clangd ─────────────────────────────────
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ─── Source files ────────────────────────────────────────────────────────────
set(ENGINE_SOURCES
    main.cpp
)

# ─── Output directory: emit .js/.wasm straight into the frontend tree ────────
set(WASM_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/../src/wasm")

add_executable(engine ${ENGINE_SOURCES})

set_target_properties(engine PROPERTIES
    OUTPUT_NAME  "engine"
    SUFFIX       ".js"
    RUNTIME_OUTPUT_DIRECTORY "${WASM_OUTPUT_DIR}"
)

# ─── Emscripten link flags ──────────────────────────────────────────────────
target_link_options(engine PRIVATE
    # Produce an ES6 module so Vite can import it directly
    "SHELL:-s MODULARIZE=1"
    "SHELL:-s EXPORT_ES6=1"
    # embind for C++ ↔ JS bindings
    "-lembind"
    # Allow the WASM heap to grow at runtime
    "SHELL:-s ALLOW_MEMORY_GROWTH=1"
    # Target WebGL 2 (maps to OpenGL ES 3.0)
    "SHELL:-s MIN_WEBGL_VERSION=2"
    "SHELL:-s MAX_WEBGL_VERSION=2"
    # Export the GL initialisation helper so JS can pass in a canvas
    "SHELL:-s EXPORTED_RUNTIME_METHODS=['GL','callMain']"
    # Keep filesystem support off to reduce binary size
    "SHELL:-s FILESYSTEM=0"
    # Environment is the browser (web worker compatible too)
    "SHELL:-s ENVIRONMENT=web,worker"
)

# ─── Copy compile_commands.json to project root for clangd ──────────────────
# CMake generates it inside the build tree; this post-build step keeps the
# one at the repo root always fresh.
add_custom_command(TARGET engine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_BINARY_DIR}/compile_commands.json"
        "${CMAKE_SOURCE_DIR}/../build/compile_commands.json"
    COMMENT "Copied compile_commands.json → build/"
)
