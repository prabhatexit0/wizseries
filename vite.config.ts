import { defineConfig } from "vite";
import react from "@vitejs/plugin-react";
import tailwindcss from "@tailwindcss/vite";
import path from "node:path";

export default defineConfig({
  plugins: [react(), tailwindcss()],

  resolve: {
    alias: {
      "@": path.resolve(__dirname, "src"),
    },
  },

  // ─── WASM handling ──────────────────────────────────────────────────────
  // Exclude the Emscripten-generated ES6 module from dependency pre-bundling.
  // Vite's optimizer does not handle .wasm sidecar loading correctly.
  optimizeDeps: {
    exclude: ["src/wasm/engine.js"],
  },

  server: {
    // Required headers so SharedArrayBuffer works (needed for WASM threads).
    headers: {
      "Cross-Origin-Opener-Policy": "same-origin",
      "Cross-Origin-Embedder-Policy": "require-corp",
    },
    fs: {
      // Allow serving the .wasm binary from src/wasm/
      allow: [".."],
    },
  },

  build: {
    // Ensure .wasm files are treated as assets and not inlined.
    assetsInlineLimit: 0,
  },
});
