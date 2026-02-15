/**
 * Stub engine loader — replaced by the real Emscripten output after running:
 *   npm run wasm:configure && npm run wasm:build
 *
 * This file lets `npm run dev` and `npm run build` succeed before the C++
 * WASM build has been executed, so the frontend toolchain works standalone.
 */
export default function createEngineModule() {
  return Promise.reject(
    new Error(
      "WASM engine not built yet. Run: npm run wasm:configure && npm run wasm:build",
    ),
  );
}
