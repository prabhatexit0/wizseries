import { useCallback, useEffect, useRef, useState } from "react";
import { useWasmEngine } from "./hooks/useWasmEngine";
import type { SeriesManager } from "./wasm";
import {
  Loader2,
  CircleCheck,
  CircleX,
  Activity,
  Infinity,
  ChevronDown,
} from "lucide-react";

import { Badge } from "@/components/ui/badge";
import {
  Card,
  CardContent,
  CardHeader,
  CardTitle,
} from "@/components/ui/card";
import { Label } from "@/components/ui/label";
import { Separator } from "@/components/ui/separator";

// ─── Canvas id shared between React and the C++ engine ──────────────────────

const CANVAS_ID = "wiz-canvas";

// ─── Visualizer catalogue ───────────────────────────────────────────────────

type VisualizerName = "cantor" | "harmonic" | "geometric" | "logistic";

interface ParamDef {
  name: string;
  label: string;
  min: number;
  max: number;
  step: number;
  default: number;
}

interface VisualizerConfig {
  label: string;
  description: string;
  params: ParamDef[];
}

const VISUALIZERS: Record<VisualizerName, VisualizerConfig> = {
  cantor: {
    label: "Cantor Set",
    description:
      "Recursive removal of middle-thirds, revealing the uncountably infinite Cantor dust at every level of depth.",
    params: [
      { name: "depth", label: "Depth", min: 1, max: 10, step: 1, default: 6 },
    ],
  },
  harmonic: {
    label: "Harmonic Series",
    description:
      "The sum 1 + 1/2 + 1/3 + \u2026 diverges to infinity, yet each successive term shrinks toward zero. The cyan line tracks the ever-growing partial sum.",
    params: [
      {
        name: "terms",
        label: "Terms",
        min: 1,
        max: 200,
        step: 1,
        default: 30,
      },
    ],
  },
  geometric: {
    label: "Geometric Series",
    description:
      "Each term is r\u00D7 the previous. When |r|<1 the series converges; when |r|\u22651 it diverges. Negative ratios produce alternating signs.",
    params: [
      {
        name: "ratio",
        label: "Ratio (r)",
        min: -1.5,
        max: 1.5,
        step: 0.01,
        default: 0.7,
      },
      {
        name: "terms",
        label: "Terms",
        min: 1,
        max: 40,
        step: 1,
        default: 15,
      },
    ],
  },
  logistic: {
    label: "Logistic Map",
    description:
      "The bifurcation diagram of x\u2099\u208A\u2081 = r\u00B7x\u2099(1\u2212x\u2099). Period-doubling cascades into chaos; the red line marks the onset at r \u2248 3.57.",
    params: [
      {
        name: "growth_rate",
        label: "Growth Rate (r)",
        min: 2.5,
        max: 4.0,
        step: 0.01,
        default: 4.0,
      },
    ],
  },
};

const VIZ_KEYS = Object.keys(VISUALIZERS) as VisualizerName[];

// ─── Build initial param state from defaults ────────────────────────────────

function buildDefaults(): Record<string, Record<string, number>> {
  const out: Record<string, Record<string, number>> = {};
  for (const [key, cfg] of Object.entries(VISUALIZERS)) {
    out[key] = {};
    for (const p of cfg.params) out[key][p.name] = p.default;
  }
  return out;
}

// ─── App ────────────────────────────────────────────────────────────────────

export default function App() {
  const { state, engine } = useWasmEngine();
  const canvasRef = useRef<HTMLCanvasElement>(null);
  const managerRef = useRef<SeriesManager | null>(null);
  const animRef = useRef<number>(0);
  const t0Ref = useRef<number>(0);

  const [glReady, setGlReady] = useState(false);
  const [activeViz, setActiveViz] = useState<VisualizerName>("cantor");
  const [paramValues, setParamValues] = useState(buildDefaults);

  // Keep a ref so the visualizer-switch effect can read the latest params
  // without re-running every time a slider moves.
  const paramValuesRef = useRef(paramValues);
  paramValuesRef.current = paramValues;

  // ── Initialise WebGL context + animation loop ──────────────────────────

  useEffect(() => {
    if (state.status !== "ready" || !engine || !canvasRef.current) return;

    // Set the canvas buffer to match its CSS layout size
    const canvas = canvasRef.current;
    canvas.width = canvas.clientWidth;
    canvas.height = canvas.clientHeight;

    const mgr = new engine.SeriesManager();
    managerRef.current = mgr;

    const ok = mgr.initGL(CANVAS_ID);
    setGlReady(ok);

    if (ok) {
      t0Ref.current = performance.now() / 1000;

      const loop = () => {
        const t = performance.now() / 1000 - t0Ref.current;
        const c = canvasRef.current;
        if (c && managerRef.current) {
          managerRef.current.render(t, c.width, c.height);
        }
        animRef.current = requestAnimationFrame(loop);
      };
      animRef.current = requestAnimationFrame(loop);
    }

    return () => {
      cancelAnimationFrame(animRef.current);
      if (managerRef.current) {
        managerRef.current.delete();
        managerRef.current = null;
      }
    };
  }, [state.status, engine]);

  // ── Resize canvas buffer on window resize ──────────────────────────────

  useEffect(() => {
    const onResize = () => {
      const c = canvasRef.current;
      if (!c) return;
      c.width = c.clientWidth;
      c.height = c.clientHeight;
    };
    window.addEventListener("resize", onResize);
    return () => window.removeEventListener("resize", onResize);
  }, []);

  // ── Switch active visualizer and push saved params ─────────────────────

  useEffect(() => {
    const mgr = managerRef.current;
    if (!mgr) return;

    mgr.setActiveVisualizer(activeViz);
    t0Ref.current = performance.now() / 1000; // restart entry animation

    const saved = paramValuesRef.current[activeViz];
    if (saved) {
      for (const [k, v] of Object.entries(saved)) mgr.setParam(k, v);
    }
  }, [activeViz]);

  // ── Slider change handler ──────────────────────────────────────────────

  const handleParam = useCallback(
    (name: string, value: number) => {
      setParamValues((prev) => ({
        ...prev,
        [activeViz]: { ...prev[activeViz], [name]: value },
      }));
      managerRef.current?.setParam(name, value);
    },
    [activeViz],
  );

  // ── Derived state ──────────────────────────────────────────────────────

  const config = VISUALIZERS[activeViz];
  const curParams = paramValues[activeViz] ?? {};

  // ── Render ─────────────────────────────────────────────────────────────

  return (
    <div className="flex h-screen overflow-hidden bg-background">
      {/* ── Sidebar ──────────────────────────────────────────────────── */}
      <aside className="w-80 shrink-0 border-r border-border bg-card flex flex-col">
        {/* Brand */}
        <div className="px-6 pt-6 pb-4 space-y-1">
          <h1 className="text-2xl font-bold tracking-tight flex items-center gap-2">
            <Infinity className="size-6" />
            WizSeries
          </h1>
          <p className="text-xs text-muted-foreground">
            Explore infinity, divergence &amp; chaos
          </p>
        </div>

        <Separator />

        <div className="flex-1 overflow-y-auto px-6 py-5 space-y-6">
          {/* Engine status badge */}
          <div>
            {state.status === "loading" && (
              <Badge variant="secondary" className="gap-1.5">
                <Loader2 className="size-3 animate-spin" />
                Loading engine…
              </Badge>
            )}
            {state.status === "error" && (
              <Badge variant="destructive" className="gap-1.5">
                <CircleX className="size-3" />
                {state.error}
              </Badge>
            )}
            {state.status === "ready" && (
              <Badge
                variant="outline"
                className="gap-1.5 border-emerald-500/50 text-emerald-600 dark:text-emerald-400"
              >
                <CircleCheck className="size-3" />
                {glReady ? "Rendering" : "Engine ready"}
              </Badge>
            )}
          </div>

          {/* Visualizer selector */}
          <div className="space-y-2">
            <Label htmlFor="viz-select">Active Series</Label>
            <div className="relative">
              <select
                id="viz-select"
                value={activeViz}
                onChange={(e) =>
                  setActiveViz(e.target.value as VisualizerName)
                }
                disabled={state.status !== "ready"}
                className="w-full appearance-none rounded-md border border-input bg-background px-3 py-2 pr-8 text-sm shadow-sm ring-offset-background focus:outline-none focus:ring-2 focus:ring-ring focus:ring-offset-2 disabled:opacity-50"
              >
                {VIZ_KEYS.map((key) => (
                  <option key={key} value={key}>
                    {VISUALIZERS[key].label}
                  </option>
                ))}
              </select>
              <ChevronDown className="pointer-events-none absolute right-2.5 top-1/2 size-4 -translate-y-1/2 text-muted-foreground" />
            </div>
          </div>

          {/* Info card */}
          <Card className="bg-muted/40">
            <CardHeader className="pb-2">
              <CardTitle className="text-sm flex items-center gap-2">
                <Activity className="size-4 text-primary" />
                {config.label}
              </CardTitle>
            </CardHeader>
            <CardContent>
              <p className="text-xs leading-relaxed text-muted-foreground">
                {config.description}
              </p>
            </CardContent>
          </Card>

          {/* Parameter sliders */}
          <div className="space-y-5">
            <span className="text-[11px] font-medium uppercase tracking-widest text-muted-foreground">
              Parameters
            </span>

            {config.params.map((p) => {
              const val = curParams[p.name] ?? p.default;
              return (
                <div key={`${activeViz}-${p.name}`} className="space-y-2">
                  <div className="flex items-center justify-between">
                    <Label
                      htmlFor={`p-${p.name}`}
                      className="text-sm font-medium"
                    >
                      {p.label}
                    </Label>
                    <span className="rounded bg-muted px-2 py-0.5 font-mono text-xs text-muted-foreground">
                      {p.step < 1 ? val.toFixed(2) : val.toFixed(0)}
                    </span>
                  </div>

                  <input
                    id={`p-${p.name}`}
                    type="range"
                    min={p.min}
                    max={p.max}
                    step={p.step}
                    value={val}
                    onChange={(e) =>
                      handleParam(p.name, parseFloat(e.target.value))
                    }
                    disabled={!glReady}
                    className="w-full cursor-pointer accent-primary"
                  />

                  <div className="flex justify-between text-[10px] text-muted-foreground/60">
                    <span>{p.min}</span>
                    <span>{p.max}</span>
                  </div>
                </div>
              );
            })}
          </div>
        </div>

        <Separator />

        <div className="px-6 py-3 text-[11px] text-muted-foreground/50">
          C++20 &middot; WebGL 2 &middot; React &middot; Emscripten
        </div>
      </aside>

      {/* ── Canvas ───────────────────────────────────────────────────── */}
      <main className="flex-1 min-w-0">
        <canvas
          ref={canvasRef}
          id={CANVAS_ID}
          className="block h-full w-full"
        />
      </main>
    </div>
  );
}
