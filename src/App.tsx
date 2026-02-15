import { useCallback, useRef, useState } from "react";
import { useWasmEngine } from "./hooks/useWasmEngine";
import {
  Cpu,
  Loader2,
  CircleCheck,
  CircleX,
  Monitor,
  Palette,
  Shuffle,
  Zap,
} from "lucide-react";

import { Button } from "@/components/ui/button";
import {
  Card,
  CardContent,
  CardDescription,
  CardHeader,
  CardTitle,
} from "@/components/ui/card";
import { Input } from "@/components/ui/input";
import { Badge } from "@/components/ui/badge";
import { Separator } from "@/components/ui/separator";
import { Label } from "@/components/ui/label";

const CANVAS_ID = "gl-canvas";
const CANVAS_WIDTH = 640;
const CANVAS_HEIGHT = 400;

export default function App() {
  const { state, engine } = useWasmEngine();
  const canvasRef = useRef<HTMLCanvasElement>(null);
  const [glReady, setGlReady] = useState(false);
  const [computeResult, setComputeResult] = useState<string | null>(null);
  const [primeLimit, setPrimeLimit] = useState(1_000_000);

  const handleInitWebGL = useCallback(() => {
    if (!engine || !canvasRef.current) return;
    const ok = engine.initWebGL(CANVAS_ID);
    setGlReady(ok);
    if (!ok) {
      setComputeResult("Failed to create WebGL 2 context.");
    }
  }, [engine]);

  const handleRender = useCallback(
    (r: number, g: number, b: number) => {
      if (!engine || !glReady) return;
      engine.renderFrame(r, g, b);
    },
    [engine, glReady],
  );

  const handleCompute = useCallback(() => {
    if (!engine) return;
    const t0 = performance.now();
    const result = engine.computePrimes(primeLimit);
    const elapsed = (performance.now() - t0).toFixed(2);
    setComputeResult(`${result}\nComputed in ${elapsed} ms`);
  }, [engine, primeLimit]);

  return (
    <div className="min-h-screen bg-background">
      <div className="mx-auto max-w-4xl px-4 py-8 space-y-6">
        {/* Header */}
        <div className="space-y-2">
          <h1 className="text-3xl font-bold tracking-tight">
            React + C++20 WASM
          </h1>
          <p className="text-muted-foreground">
            High-performance C++20 compiled to WebAssembly, integrated with
            React via Emscripten and embind.
          </p>
          <div className="pt-1">
            {state.status === "loading" && (
              <Badge variant="secondary" className="gap-1.5">
                <Loader2 className="size-3 animate-spin" />
                Loading WASM engine...
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
                Engine ready
              </Badge>
            )}
          </div>
        </div>

        <Separator />

        {/* WebGL Card */}
        <Card>
          <CardHeader>
            <div className="flex items-center gap-2">
              <Monitor className="size-5 text-muted-foreground" />
              <CardTitle>WebGL 2 Canvas</CardTitle>
            </div>
            <CardDescription>
              Initialise a WebGL 2 rendering context and clear the canvas with
              different colours — all driven from C++ via Emscripten.
            </CardDescription>
          </CardHeader>
          <CardContent className="space-y-4">
            <canvas
              ref={canvasRef}
              id={CANVAS_ID}
              width={CANVAS_WIDTH}
              height={CANVAS_HEIGHT}
              className="w-full max-w-[640px] rounded-lg border bg-muted/30"
            />
            <div className="flex flex-wrap gap-2">
              <Button
                onClick={handleInitWebGL}
                disabled={state.status !== "ready" || glReady}
                variant={glReady ? "secondary" : "default"}
              >
                {glReady ? (
                  <>
                    <CircleCheck className="size-4" />
                    WebGL Initialised
                  </>
                ) : (
                  <>
                    <Zap className="size-4" />
                    Init WebGL
                  </>
                )}
              </Button>

              <Separator orientation="vertical" className="h-9" />

              <Button
                variant="outline"
                onClick={() => handleRender(0.39, 0.39, 1.0)}
                disabled={!glReady}
              >
                <Palette className="size-4" />
                Blue
              </Button>
              <Button
                variant="outline"
                onClick={() => handleRender(0.2, 0.8, 0.4)}
                disabled={!glReady}
              >
                <Palette className="size-4" />
                Green
              </Button>
              <Button
                variant="outline"
                onClick={() => handleRender(0.9, 0.3, 0.3)}
                disabled={!glReady}
              >
                <Palette className="size-4" />
                Red
              </Button>
              <Button
                variant="outline"
                onClick={() =>
                  handleRender(Math.random(), Math.random(), Math.random())
                }
                disabled={!glReady}
              >
                <Shuffle className="size-4" />
                Random
              </Button>
            </div>
          </CardContent>
        </Card>

        {/* Compute Card */}
        <Card>
          <CardHeader>
            <div className="flex items-center gap-2">
              <Cpu className="size-5 text-muted-foreground" />
              <CardTitle>C++20 Prime Sieve</CardTitle>
            </div>
            <CardDescription>
              Run a Sieve of Eratosthenes implemented with C++20{" "}
              <code className="text-xs bg-muted px-1.5 py-0.5 rounded">
                std::views
              </code>{" "}
              and{" "}
              <code className="text-xs bg-muted px-1.5 py-0.5 rounded">
                std::ranges
              </code>
              . The computation runs entirely in WebAssembly.
            </CardDescription>
          </CardHeader>
          <CardContent className="space-y-4">
            <div className="flex items-end gap-3">
              <div className="space-y-2">
                <Label htmlFor="prime-limit">Upper limit</Label>
                <Input
                  id="prime-limit"
                  type="number"
                  value={primeLimit}
                  onChange={(e) => setPrimeLimit(Number(e.target.value))}
                  min={2}
                  max={100_000_000}
                  className="w-44 font-mono"
                />
              </div>
              <Button
                onClick={handleCompute}
                disabled={state.status !== "ready"}
              >
                <Zap className="size-4" />
                Compute Primes
              </Button>
            </div>

            {computeResult && (
              <pre className="rounded-lg border bg-muted/50 p-4 text-sm font-mono leading-relaxed whitespace-pre-wrap">
                {computeResult}
              </pre>
            )}
          </CardContent>
        </Card>
      </div>
    </div>
  );
}
