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
  Menu,
  X,
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
      "The sum 1 + 1/2 + 1/3 + \u2026 diverges to infinity, yet each successive term shrinks toward zero. The blue line tracks the ever-growing partial sum.",
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

// ─── Coordinate helpers ─────────────────────────────────────────────────────
// Convert clip-space (-1..1) to canvas pixel coordinates.

function clipToPixelX(clipX: number, w: number): number {
  return ((clipX + 1) / 2) * w;
}
function clipToPixelY(clipY: number, h: number): number {
  return ((1 - clipY) / 2) * h;
}

// ─── Annotation renderers ───────────────────────────────────────────────────
// Each draws text labels on a transparent 2D canvas overlay that sits on top
// of the WebGL canvas.  The clip-space margins match the C++ visualizer code.

const LABEL_COLOR = "#3d3a37";
const LABEL_MUTED = "#7a7672";
const FORMULA_COLOR = "#1a5276";
const ACCENT_COLOR = "#8e44ad";
const SUM_COLOR = "#1a3f7a";
const LIMIT_COLOR = "#1a7a2e";

function drawCantorAnnotations(
  ctx: CanvasRenderingContext2D,
  w: number,
  h: number,
  params: Record<string, number>,
) {
  const depth = Math.min(12, Math.max(1, Math.round(params.depth ?? 6)));

  const mLeft = 0.14, mRight = 0.06, mBottom = 0.10, mTop = 0.08;
  const xMin = -1 + mLeft, xMax = 1 - mRight;
  const yMin = -1 + mBottom, yMax = 1 - mTop;

  const totalH = yMax - yMin;
  const gap = totalH / (depth + 1);
  const barH = gap * 0.70;

  const baseFontSize = Math.max(10, Math.min(14, w * 0.012));
  ctx.textBaseline = "middle";

  // Level labels on the left axis
  ctx.font = `${baseFontSize}px "SF Mono", "Cascadia Code", "Fira Code", monospace`;
  ctx.fillStyle = LABEL_COLOR;
  ctx.textAlign = "right";
  for (let lv = 0; lv <= depth; lv++) {
    const y = yMax - lv * gap - barH * 0.5;
    const px = clipToPixelX(xMin - 0.025, w);
    const py = clipToPixelY(y, h);
    ctx.fillText(`${lv}`, px, py);
  }

  // Segments count on the right side
  ctx.font = `${baseFontSize * 0.9}px "SF Mono", "Cascadia Code", "Fira Code", monospace`;
  ctx.fillStyle = LABEL_MUTED;
  ctx.textAlign = "left";
  for (let lv = 0; lv <= depth; lv++) {
    const y = yMax - lv * gap - barH * 0.5;
    const px = clipToPixelX(xMax + 0.02, w);
    const py = clipToPixelY(y, h);
    const segs = Math.pow(2, lv);
    ctx.fillText(`${segs} seg${segs > 1 ? "s" : ""}`, px, py);
  }

  // Y-axis label
  ctx.save();
  ctx.font = `bold ${baseFontSize}px system-ui, sans-serif`;
  ctx.fillStyle = LABEL_COLOR;
  ctx.textAlign = "center";
  const ylX = clipToPixelX(-1 + 0.03, w);
  const ylY = clipToPixelY(0, h);
  ctx.translate(ylX, ylY);
  ctx.rotate(-Math.PI / 2);
  ctx.fillText("Level", 0, 0);
  ctx.restore();

  // Title / formula
  ctx.font = `${baseFontSize * 1.1}px system-ui, sans-serif`;
  ctx.fillStyle = FORMULA_COLOR;
  ctx.textAlign = "center";
  ctx.fillText(
    `Cantor Set  \u2014  depth = ${depth}`,
    clipToPixelX((xMin + xMax) / 2, w),
    clipToPixelY(yMax + 0.05, h),
  );

  // Bottom note: measure info
  ctx.font = `italic ${baseFontSize * 0.9}px system-ui, sans-serif`;
  ctx.fillStyle = LABEL_MUTED;
  ctx.fillText(
    `Total length remaining: (2/3)${superscript(depth)} \u2248 ${Math.pow(2 / 3, depth).toFixed(4)}`,
    clipToPixelX((xMin + xMax) / 2, w),
    clipToPixelY(yMin - 0.055, h),
  );
}

function drawHarmonicAnnotations(
  ctx: CanvasRenderingContext2D,
  w: number,
  h: number,
  params: Record<string, number>,
) {
  const terms = Math.min(500, Math.max(1, Math.round(params.terms ?? 30)));

  const mLeft = 0.14, mRight = 0.06, mBottom = 0.12, mTop = 0.08;
  const xMin = -1 + mLeft, xMax = 1 - mRight;
  const yMin = -1 + mBottom, yMax = 1 - mTop;

  // Compute partial sum and yScale (matching C++)
  let maxSum = 0;
  for (let k = 1; k <= terms; k++) maxSum += 1 / k;
  const yScale = Math.max(1, maxSum) * 1.1;

  const baseFontSize = Math.max(10, Math.min(14, w * 0.012));

  // Y-axis tick labels
  let step = 1;
  if (yScale > 8) step = 2;
  if (yScale > 16) step = 4;

  ctx.font = `${baseFontSize}px "SF Mono", "Cascadia Code", "Fira Code", monospace`;
  ctx.fillStyle = LABEL_COLOR;
  ctx.textAlign = "right";
  ctx.textBaseline = "middle";
  for (let v = 0; v < yScale; v += step) {
    const clipY = yMin + (v / yScale) * (yMax - yMin);
    const px = clipToPixelX(xMin - 0.025, w);
    const py = clipToPixelY(clipY, h);
    ctx.fillText(v.toFixed(0), px, py);
  }

  // X-axis: show term indices
  ctx.textAlign = "center";
  ctx.textBaseline = "top";
  const barW = (xMax - xMin) / terms;
  // Pick sensible x-tick spacing
  let xStep = 1;
  if (terms > 10) xStep = 5;
  if (terms > 50) xStep = 10;
  if (terms > 100) xStep = 25;
  for (let k = 1; k <= terms; k += xStep) {
    const cx = xMin + (k - 0.5) * barW;
    const px = clipToPixelX(cx, w);
    const py = clipToPixelY(yMin - 0.02, h);
    ctx.fillText(`${k}`, px, py);
  }

  // Y-axis label
  ctx.save();
  ctx.font = `bold ${baseFontSize}px system-ui, sans-serif`;
  ctx.fillStyle = LABEL_COLOR;
  ctx.textAlign = "center";
  ctx.textBaseline = "middle";
  const ylX = clipToPixelX(-1 + 0.03, w);
  const ylY = clipToPixelY((yMin + yMax) / 2, h);
  ctx.translate(ylX, ylY);
  ctx.rotate(-Math.PI / 2);
  ctx.fillText("Value", 0, 0);
  ctx.restore();

  // X-axis label
  ctx.font = `bold ${baseFontSize}px system-ui, sans-serif`;
  ctx.fillStyle = LABEL_COLOR;
  ctx.textAlign = "center";
  ctx.textBaseline = "top";
  ctx.fillText(
    "Term (k)",
    clipToPixelX((xMin + xMax) / 2, w),
    clipToPixelY(yMin - 0.07, h),
  );

  // Title / formula
  ctx.font = `${baseFontSize * 1.1}px system-ui, sans-serif`;
  ctx.fillStyle = FORMULA_COLOR;
  ctx.textAlign = "center";
  ctx.textBaseline = "bottom";
  ctx.fillText(
    `H\u2099 = \u2211 1/k,  k = 1\u2026${terms}`,
    clipToPixelX((xMin + xMax) / 2, w),
    clipToPixelY(yMax + 0.05, h),
  );

  // Partial sum value — top right corner
  ctx.font = `bold ${baseFontSize * 1.05}px "SF Mono", "Cascadia Code", "Fira Code", monospace`;
  ctx.fillStyle = SUM_COLOR;
  ctx.textAlign = "right";
  ctx.textBaseline = "top";
  ctx.fillText(
    `S${subscriptDigits(terms)} = ${maxSum.toFixed(4)}`,
    clipToPixelX(xMax - 0.01, w),
    clipToPixelY(yMax + 0.04, h),
  );

  // Divergence note
  ctx.font = `italic ${baseFontSize * 0.85}px system-ui, sans-serif`;
  ctx.fillStyle = ACCENT_COLOR;
  ctx.textAlign = "right";
  ctx.fillText(
    "Diverges  \u2192  \u221E",
    clipToPixelX(xMax - 0.01, w),
    clipToPixelY(yMax + 0.04, h) + baseFontSize * 1.4,
  );
}

function drawGeometricAnnotations(
  ctx: CanvasRenderingContext2D,
  w: number,
  h: number,
  params: Record<string, number>,
) {
  const ratio = params.ratio ?? 0.7;
  const terms = Math.min(50, Math.max(1, Math.round(params.terms ?? 15)));

  const mLeft = 0.14, mRight = 0.06, mBottom = 0.12, mTop = 0.08;
  const xMin = -1 + mLeft, xMax = 1 - mRight;
  const yMid = 0;
  const yExt = 1 - Math.max(mTop, mBottom);

  // Pre-scan for scaling (matching C++)
  let maxAbsVal = 0, maxAbsSum = 0;
  {
    let v = 1, s = 0;
    for (let k = 0; k < terms; k++) {
      maxAbsVal = Math.max(maxAbsVal, Math.abs(v));
      s += v;
      maxAbsSum = Math.max(maxAbsSum, Math.abs(s));
      v *= ratio;
    }
  }
  const scale = Math.max(maxAbsVal, maxAbsSum, 0.001);

  // Compute final partial sum
  let partialSum = 0;
  {
    let v = 1;
    for (let k = 0; k < terms; k++) {
      partialSum += v;
      v *= ratio;
    }
  }

  const baseFontSize = Math.max(10, Math.min(14, w * 0.012));
  ctx.textBaseline = "middle";

  // Y-axis tick labels (symmetric around 0)
  let tickStep = scale / 4;
  if (tickStep < 0.01) tickStep = 0.01;
  const mag = Math.pow(10, Math.floor(Math.log10(tickStep)));
  tickStep = Math.ceil(tickStep / mag) * mag;

  ctx.font = `${baseFontSize}px "SF Mono", "Cascadia Code", "Fira Code", monospace`;
  ctx.fillStyle = LABEL_COLOR;
  ctx.textAlign = "right";

  // Zero label
  ctx.fillText(
    "0",
    clipToPixelX(xMin - 0.025, w),
    clipToPixelY(yMid, h),
  );

  for (let v = tickStep; v < scale; v += tickStep) {
    // Positive
    const cyP = yMid + (v / scale) * yExt;
    ctx.fillText(
      formatTick(v),
      clipToPixelX(xMin - 0.025, w),
      clipToPixelY(cyP, h),
    );
    // Negative
    const cyN = yMid - (v / scale) * yExt;
    ctx.fillText(
      formatTick(-v),
      clipToPixelX(xMin - 0.025, w),
      clipToPixelY(cyN, h),
    );
  }

  // X-axis: term indices
  ctx.textAlign = "center";
  ctx.textBaseline = "top";
  const barW = (xMax - xMin) / terms;
  let xStep = 1;
  if (terms > 10) xStep = 2;
  if (terms > 20) xStep = 5;
  for (let k = 0; k < terms; k += xStep) {
    const cx = xMin + (k + 0.5) * barW;
    const px = clipToPixelX(cx, w);
    // Place below the zero line or below the bottom axis
    const py = clipToPixelY(-1 + mBottom - 0.02, h);
    ctx.fillStyle = LABEL_COLOR;
    ctx.fillText(`${k}`, px, py);
  }

  // Y-axis label
  ctx.save();
  ctx.font = `bold ${baseFontSize}px system-ui, sans-serif`;
  ctx.fillStyle = LABEL_COLOR;
  ctx.textAlign = "center";
  ctx.textBaseline = "middle";
  const ylX = clipToPixelX(-1 + 0.025, w);
  const ylY = clipToPixelY(0, h);
  ctx.translate(ylX, ylY);
  ctx.rotate(-Math.PI / 2);
  ctx.fillText("Value", 0, 0);
  ctx.restore();

  // X-axis label
  ctx.font = `bold ${baseFontSize}px system-ui, sans-serif`;
  ctx.fillStyle = LABEL_COLOR;
  ctx.textAlign = "center";
  ctx.textBaseline = "top";
  ctx.fillText(
    "Term (k)",
    clipToPixelX((xMin + xMax) / 2, w),
    clipToPixelY(-1 + mBottom - 0.07, h),
  );

  // Title / formula
  ctx.font = `${baseFontSize * 1.1}px system-ui, sans-serif`;
  ctx.fillStyle = FORMULA_COLOR;
  ctx.textAlign = "center";
  ctx.textBaseline = "bottom";
  ctx.fillText(
    `\u2211 r\u1D4F,  r = ${ratio.toFixed(2)},  k = 0\u2026${terms - 1}`,
    clipToPixelX((xMin + xMax) / 2, w),
    clipToPixelY(1 - mTop + 0.05, h),
  );

  // Partial sum — top right
  ctx.font = `bold ${baseFontSize * 1.05}px "SF Mono", "Cascadia Code", "Fira Code", monospace`;
  ctx.fillStyle = SUM_COLOR;
  ctx.textAlign = "right";
  ctx.textBaseline = "top";
  ctx.fillText(
    `S${subscriptDigits(terms)} = ${partialSum.toFixed(4)}`,
    clipToPixelX(xMax - 0.01, w),
    clipToPixelY(1 - mTop + 0.04, h),
  );

  // Convergence limit
  if (Math.abs(ratio) < 1 && Math.abs(1 - ratio) > 1e-6) {
    const limit = 1 / (1 - ratio);
    ctx.font = `bold ${baseFontSize}px "SF Mono", "Cascadia Code", "Fira Code", monospace`;
    ctx.fillStyle = LIMIT_COLOR;
    ctx.textAlign = "right";
    ctx.fillText(
      `Limit = 1/(1\u2212r) = ${limit.toFixed(4)}`,
      clipToPixelX(xMax - 0.01, w),
      clipToPixelY(1 - mTop + 0.04, h) + baseFontSize * 1.5,
    );
  } else {
    ctx.font = `italic ${baseFontSize * 0.85}px system-ui, sans-serif`;
    ctx.fillStyle = ACCENT_COLOR;
    ctx.textAlign = "right";
    ctx.fillText(
      "|r| \u2265 1  \u2014  Diverges",
      clipToPixelX(xMax - 0.01, w),
      clipToPixelY(1 - mTop + 0.04, h) + baseFontSize * 1.5,
    );
  }
}

function drawLogisticAnnotations(
  ctx: CanvasRenderingContext2D,
  w: number,
  h: number,
  params: Record<string, number>,
) {
  const rMax = Math.min(4, Math.max(1, params.growth_rate ?? 4));
  const rMin = 1.0;

  const mLeft = 0.14, mRight = 0.06, mBottom = 0.12, mTop = 0.08;
  const xMin = -1 + mLeft, xMax = 1 - mRight;
  const yMin = -1 + mBottom, yMax = 1 - mTop;

  const baseFontSize = Math.max(10, Math.min(14, w * 0.012));

  // X-axis (r) tick labels
  ctx.font = `${baseFontSize}px "SF Mono", "Cascadia Code", "Fira Code", monospace`;
  ctx.fillStyle = LABEL_COLOR;
  ctx.textAlign = "center";
  ctx.textBaseline = "top";
  for (const rv of [1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0]) {
    if (rv < rMin || rv > rMax + 0.01) continue;
    const t = (rv - rMin) / (rMax - rMin);
    const px = clipToPixelX(xMin + (xMax - xMin) * t, w);
    const py = clipToPixelY(yMin - 0.025, h);
    ctx.fillText(rv.toFixed(1), px, py);
  }

  // Chaos onset label
  if (rMax > 3.57) {
    const chaosT = (3.57 - rMin) / (rMax - rMin);
    const cx = xMin + (xMax - xMin) * chaosT;
    ctx.font = `italic ${baseFontSize * 0.85}px system-ui, sans-serif`;
    ctx.fillStyle = "#a82020";
    ctx.textAlign = "left";
    ctx.textBaseline = "bottom";
    ctx.fillText(
      "r \u2248 3.57",
      clipToPixelX(cx + 0.01, w),
      clipToPixelY(yMax - 0.02, h),
    );
    ctx.fillText(
      "(chaos)",
      clipToPixelX(cx + 0.01, w),
      clipToPixelY(yMax - 0.02, h) + baseFontSize * 1.1,
    );
  }

  // Y-axis (x) tick labels
  ctx.font = `${baseFontSize}px "SF Mono", "Cascadia Code", "Fira Code", monospace`;
  ctx.fillStyle = LABEL_COLOR;
  ctx.textAlign = "right";
  ctx.textBaseline = "middle";
  for (const v of [0, 0.25, 0.5, 0.75, 1.0]) {
    const clipY = yMin + (yMax - yMin) * v;
    ctx.fillText(
      v.toFixed(2),
      clipToPixelX(xMin - 0.02, w),
      clipToPixelY(clipY, h),
    );
  }

  // Axis labels
  ctx.font = `bold ${baseFontSize}px system-ui, sans-serif`;
  ctx.fillStyle = LABEL_COLOR;
  ctx.textAlign = "center";
  ctx.textBaseline = "top";
  ctx.fillText(
    "Growth rate (r)",
    clipToPixelX((xMin + xMax) / 2, w),
    clipToPixelY(yMin - 0.075, h),
  );

  ctx.save();
  ctx.textBaseline = "middle";
  const ylX = clipToPixelX(-1 + 0.025, w);
  const ylY = clipToPixelY((yMin + yMax) / 2, h);
  ctx.translate(ylX, ylY);
  ctx.rotate(-Math.PI / 2);
  ctx.fillText("x\u2099", 0, 0);
  ctx.restore();

  // Title / formula
  ctx.font = `${baseFontSize * 1.1}px system-ui, sans-serif`;
  ctx.fillStyle = FORMULA_COLOR;
  ctx.textAlign = "center";
  ctx.textBaseline = "bottom";
  ctx.fillText(
    "x\u2099\u208A\u2081 = r \u00B7 x\u2099 \u00B7 (1 \u2212 x\u2099)",
    clipToPixelX((xMin + xMax) / 2, w),
    clipToPixelY(yMax + 0.05, h),
  );

  // Current rMax info — top right
  ctx.font = `bold ${baseFontSize}px "SF Mono", "Cascadia Code", "Fira Code", monospace`;
  ctx.fillStyle = SUM_COLOR;
  ctx.textAlign = "right";
  ctx.textBaseline = "top";
  ctx.fillText(
    `r \u2208 [1.00, ${rMax.toFixed(2)}]`,
    clipToPixelX(xMax - 0.01, w),
    clipToPixelY(yMax + 0.04, h),
  );
}

// ─── Text formatting helpers ────────────────────────────────────────────────

function subscriptDigits(n: number): string {
  const subs = "\u2080\u2081\u2082\u2083\u2084\u2085\u2086\u2087\u2088\u2089";
  return String(n)
    .split("")
    .map((d) => (/\d/.test(d) ? subs[parseInt(d)] : d))
    .join("");
}

function superscript(n: number): string {
  const sups: Record<string, string> = {
    "0": "\u2070",
    "1": "\u00B9",
    "2": "\u00B2",
    "3": "\u00B3",
    "4": "\u2074",
    "5": "\u2075",
    "6": "\u2076",
    "7": "\u2077",
    "8": "\u2078",
    "9": "\u2079",
  };
  return String(n)
    .split("")
    .map((d) => sups[d] ?? d)
    .join("");
}

function formatTick(v: number): string {
  if (Math.abs(v) >= 100) return v.toFixed(0);
  if (Math.abs(v) >= 1) return v.toFixed(1);
  return v.toFixed(2);
}

// ─── Annotation dispatch ────────────────────────────────────────────────────

const ANNOTATION_RENDERERS: Record<
  VisualizerName,
  (
    ctx: CanvasRenderingContext2D,
    w: number,
    h: number,
    params: Record<string, number>,
  ) => void
> = {
  cantor: drawCantorAnnotations,
  harmonic: drawHarmonicAnnotations,
  geometric: drawGeometricAnnotations,
  logistic: drawLogisticAnnotations,
};

// ─── App ────────────────────────────────────────────────────────────────────

export default function App() {
  const { state, engine } = useWasmEngine();
  const canvasRef = useRef<HTMLCanvasElement>(null);
  const overlayRef = useRef<HTMLCanvasElement>(null);
  const managerRef = useRef<SeriesManager | null>(null);
  const animRef = useRef<number>(0);
  const t0Ref = useRef<number>(0);

  const [glReady, setGlReady] = useState(false);
  const [activeViz, setActiveViz] = useState<VisualizerName>("cantor");
  const [paramValues, setParamValues] = useState(buildDefaults);
  const [sidebarOpen, setSidebarOpen] = useState(false);

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

      // Also resize the overlay
      const o = overlayRef.current;
      if (o) {
        o.width = c.clientWidth;
        o.height = c.clientHeight;
      }
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

  // ── Draw text overlay annotations ──────────────────────────────────────

  const drawAnnotations = useCallback(() => {
    const overlay = overlayRef.current;
    const canvas = canvasRef.current;
    if (!overlay || !canvas) return;

    // Ensure overlay matches canvas size
    if (overlay.width !== canvas.clientWidth || overlay.height !== canvas.clientHeight) {
      overlay.width = canvas.clientWidth;
      overlay.height = canvas.clientHeight;
    }

    const ctx = overlay.getContext("2d");
    if (!ctx) return;

    const w = overlay.width;
    const h = overlay.height;
    ctx.clearRect(0, 0, w, h);

    const renderer = ANNOTATION_RENDERERS[activeViz];
    if (renderer) {
      renderer(ctx, w, h, paramValues[activeViz] ?? {});
    }
  }, [activeViz, paramValues]);

  // Redraw annotations when viz or params change
  useEffect(() => {
    // Small delay to let canvas sizing settle
    const id = requestAnimationFrame(drawAnnotations);
    return () => cancelAnimationFrame(id);
  }, [drawAnnotations]);

  // Also redraw on resize
  useEffect(() => {
    const onResize = () => drawAnnotations();
    window.addEventListener("resize", onResize);
    return () => window.removeEventListener("resize", onResize);
  }, [drawAnnotations]);

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
      {/* ── Mobile backdrop ──────────────────────────────────────────── */}
      {sidebarOpen && (
        <div
          className="fixed inset-0 z-30 bg-black/50 md:hidden"
          onClick={() => setSidebarOpen(false)}
        />
      )}

      {/* ── Sidebar ──────────────────────────────────────────────────── */}
      <aside
        className={`
          fixed inset-y-0 left-0 z-40 w-[280px] sm:w-80 shrink-0 border-r border-border bg-card flex flex-col
          transition-transform duration-200 ease-in-out
          ${sidebarOpen ? "translate-x-0" : "-translate-x-full"}
          md:static md:translate-x-0
        `}
      >
        {/* Brand */}
        <div className="px-4 sm:px-6 pt-5 sm:pt-6 pb-3 sm:pb-4 space-y-1 flex items-start justify-between">
          <div className="space-y-1">
            <h1 className="text-xl sm:text-2xl font-bold tracking-tight flex items-center gap-2">
              <Infinity className="size-5 sm:size-6" />
              WizSeries
            </h1>
            <p className="text-xs text-muted-foreground">
              Explore infinity, divergence &amp; chaos
            </p>
          </div>
          <button
            onClick={() => setSidebarOpen(false)}
            className="md:hidden p-1.5 -mr-1.5 rounded-md hover:bg-muted"
            aria-label="Close sidebar"
          >
            <X className="size-5" />
          </button>
        </div>

        <Separator />

        <div className="flex-1 overflow-y-auto px-4 sm:px-6 py-4 sm:py-5 space-y-5 sm:space-y-6">
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
                className="gap-1.5 border-emerald-500/50 text-emerald-600"
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
                onChange={(e) => {
                  setActiveViz(e.target.value as VisualizerName);
                  setSidebarOpen(false);
                }}
                disabled={state.status !== "ready"}
                className="w-full appearance-none rounded-md border border-input bg-background px-3 py-2.5 sm:py-2 pr-8 text-sm shadow-sm ring-offset-background focus:outline-none focus:ring-2 focus:ring-ring focus:ring-offset-2 disabled:opacity-50"
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
                    className="w-full h-2 cursor-pointer accent-primary"
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

        <div className="px-4 sm:px-6 py-3 text-[11px] text-muted-foreground/50">
          C++20 &middot; WebGL 2 &middot; React &middot; Emscripten
        </div>
      </aside>

      {/* ── Canvas ───────────────────────────────────────────────────── */}
      <main className="flex-1 min-w-0 relative">
        {/* Mobile menu button */}
        <button
          onClick={() => setSidebarOpen(true)}
          className="md:hidden absolute top-3 left-3 z-20 p-2 rounded-lg bg-card/80 backdrop-blur-sm border border-border shadow-sm hover:bg-card"
          aria-label="Open sidebar"
        >
          <Menu className="size-5" />
        </button>

        <canvas
          ref={canvasRef}
          id={CANVAS_ID}
          className="block h-full w-full"
        />
        <canvas
          ref={overlayRef}
          className="absolute inset-0 block h-full w-full pointer-events-none"
        />
      </main>
    </div>
  );
}
