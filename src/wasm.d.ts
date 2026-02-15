/**
 * TypeScript declarations for the Emscripten-generated engine module.
 *
 * These types mirror the C++ functions exported via embind in cpp/main.cpp.
 * Update this file whenever you add or change exports on the C++ side.
 */

/** The instantiated WASM engine module. */
export interface EngineModule {
  /**
   * Run a Sieve of Eratosthenes up to `limit` and return a summary string
   * containing the count and the last few primes.
   */
  computePrimes(limit: number): string;

  /**
   * Initialise a WebGL 2 rendering context on the given canvas element.
   * @param canvasId  The DOM `id` attribute of the target `<canvas>` (without `#`).
   * @returns `true` if the context was created successfully.
   */
  initWebGL(canvasId: string): boolean;

  /**
   * Clear the canvas with the given RGB colour.
   * Each channel is a float in [0, 1].
   */
  renderFrame(r: number, g: number, b: number): void;
}

/**
 * Factory function exported by the Emscripten ES6 module.
 * Call it to instantiate and initialise the WASM engine.
 */
declare function createEngineModule(): Promise<EngineModule>;
export default createEngineModule;
