/**
 * TypeScript declarations for the Emscripten-generated engine module.
 *
 * These types mirror the C++ functions / classes exported via embind
 * in cpp/main.cpp.  Update this file whenever you change exports.
 */

// ─── SeriesManager (embind class) ───────────────────────────────────────────

export interface SeriesManager {
  /** Create a WebGL 2 context on the given canvas. */
  initGL(canvasId: string): boolean;

  /** Render one frame of the active visualizer. */
  render(time: number, width: number, height: number): void;

  /** Switch the active visualizer by key name. */
  setActiveVisualizer(name: string): void;

  /** Get the key name of the currently active visualizer. */
  getActiveVisualizer(): string;

  /** Set a named parameter on the active visualizer. */
  setParam(name: string, value: number): void;

  /** Set the horizontal pan/zoom view transform. */
  setView(scale: number, offsetX: number): void;

  /** Release the C++ instance (call when done). */
  delete(): void;
}

// ─── Legacy free functions ──────────────────────────────────────────────────

/** The instantiated WASM engine module. */
export interface EngineModule {
  computePrimes(limit: number): string;
  initWebGL(canvasId: string): boolean;
  renderFrame(r: number, g: number, b: number): void;

  /** Embind class constructor for SeriesManager. */
  SeriesManager: { new (): SeriesManager };
}

/**
 * Factory function exported by the Emscripten ES6 module.
 * Call it to instantiate and initialise the WASM engine.
 */
declare function createEngineModule(): Promise<EngineModule>;
export default createEngineModule;
