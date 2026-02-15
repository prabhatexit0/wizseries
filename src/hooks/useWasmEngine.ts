import { useEffect, useRef, useState } from "react";
import type { EngineModule } from "../wasm";

type EngineState =
  | { status: "loading" }
  | { status: "ready"; engine: EngineModule }
  | { status: "error"; error: string };

/**
 * React hook that lazily loads and instantiates the Emscripten WASM engine.
 *
 * Usage:
 *   const { state, engine } = useWasmEngine();
 */
export function useWasmEngine() {
  const [state, setState] = useState<EngineState>({ status: "loading" });
  const engineRef = useRef<EngineModule | null>(null);

  useEffect(() => {
    let cancelled = false;

    async function load() {
      try {
        // Dynamic import so Vite can code-split the WASM loader
        const { default: createModule } = await import("../wasm/engine.js");
        const instance: EngineModule = await createModule();

        if (!cancelled) {
          engineRef.current = instance;
          setState({ status: "ready", engine: instance });
        }
      } catch (err) {
        if (!cancelled) {
          const message =
            err instanceof Error ? err.message : "Unknown error loading WASM";
          setState({ status: "error", error: message });
        }
      }
    }

    load();
    return () => {
      cancelled = true;
    };
  }, []);

  return {
    state,
    engine: engineRef.current,
  };
}
