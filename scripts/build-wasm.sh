#!/usr/bin/env bash
# ──────────────────────────────────────────────────────────────────────────────
# Build the C++ WASM engine using Emscripten + CMake.
#
# Usage:
#   ./scripts/build-wasm.sh          # configure + build (Release)
#   ./scripts/build-wasm.sh --clean  # clean build
#   ./scripts/build-wasm.sh --debug  # configure + build (Debug)
# ──────────────────────────────────────────────────────────────────────────────
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"
CPP_DIR="${ROOT_DIR}/cpp"
BUILD_TYPE="Release"

for arg in "$@"; do
  case "$arg" in
    --clean)
      echo "Cleaning build directory..."
      rm -rf "${BUILD_DIR}" "${ROOT_DIR}/src/wasm/engine.js" "${ROOT_DIR}/src/wasm/engine.wasm"
      ;;
    --debug)
      BUILD_TYPE="Debug"
      ;;
  esac
done

# Ensure Emscripten is available
if ! command -v emcmake &> /dev/null; then
  echo "Error: emcmake not found. Please install and activate the Emscripten SDK."
  echo "  git clone https://github.com/emscripten-core/emsdk.git"
  echo "  cd emsdk && ./emsdk install latest && ./emsdk activate latest"
  echo "  source ./emsdk_env.sh"
  exit 1
fi

echo "Configuring CMake (${BUILD_TYPE})..."
emcmake cmake -S "${CPP_DIR}" -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE="${BUILD_TYPE}"

echo "Building..."
cmake --build "${BUILD_DIR}" --parallel "$(nproc 2>/dev/null || echo 4)"

echo ""
echo "Build complete. WASM output:"
ls -lh "${ROOT_DIR}/src/wasm/engine.js" "${ROOT_DIR}/src/wasm/engine.wasm" 2>/dev/null || echo "(files not found — check for errors above)"
