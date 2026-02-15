# WizSeries

Math is cool. This project proves it.

WizSeries is a playground for visualizing mathematical series and chaos theory — right in your browser. It pairs a **React** frontend with a **C++20 WebAssembly** engine so the heavy math runs at near-native speed while you get a nice UI to poke at.

## What's Inside

Four interactive visualizers you can play with:

- **Cantor Set** — The classic fractal. Remove the middle third, repeat forever, question reality.
- **Harmonic Series** — Watch 1 + 1/2 + 1/3 + ... crawl toward infinity (it gets there eventually).
- **Geometric Series** — Converges or diverges depending on the ratio. Includes a bifurcation view because why not.
- **Logistic Map** — Bifurcation diagrams, period-doubling, and the road to chaos. Surprisingly pretty.

Everything renders via **WebGL 2.0** so your GPU does the heavy lifting.

## Tech Stack

**Frontend:** React 19, TypeScript, Vite, Tailwind CSS v4, shadcn/ui

**Engine:** C++20 compiled to WebAssembly via Emscripten

**Rendering:** WebGL 2.0

## Getting Started

You'll need **Node.js >= 18**, **CMake >= 3.20**, and the **Emscripten SDK**.

```bash
# install emscripten (one-time)
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk && ./emsdk install latest && ./emsdk activate latest
source ./emsdk_env.sh

# back in the project directory — do everything in one shot
npm run setup

# start the dev server
npm run dev
```

Open `http://localhost:5173` and start exploring.

> The frontend works even without the WASM build — you'll just see an error badge nudging you to compile the engine.

## Useful Commands

| Command | What it does |
|---------|-------------|
| `npm run dev` | Start dev server |
| `npm run build` | Production build |
| `npm run wasm:build` | Rebuild the WASM engine (incremental) |
| `npm run wasm:rebuild` | Clean rebuild of the WASM engine |
| `npm run setup` | Install deps + full WASM build |

## Hacking on the C++ Side

1. Edit stuff in `cpp/`
2. Run `npm run wasm:build`
3. Refresh your browser

If you add new C++ exports, update the types in `src/wasm.d.ts` so TypeScript stays happy.

## Why

Because watching math happen in real-time is more fun than reading about it in a textbook. This is an experimental project — expect rough edges, have fun breaking things.
