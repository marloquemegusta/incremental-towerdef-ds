# Performance sandbox

The deterministic stress scenario is:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/run-scenario.ps1 `
  -RomPath game.nds `
  -ScenarioPath scenarios/performance_sandbox_stress_run.json `
  -OutputPath artifacts/performance-sandbox
```

The stress chord loads the deterministic 384-enemy profile. Stress runs use
the compact HUD so the profiler does not dominate the frame. The top-screen
telemetry means:

- `FPS`: frames per second measured by the emulated DS timer.
- `T/B/P/S/E`: top render, bottom render, presentation, simulation and active
  enemies. These are timer-0 averages over the last profiler window.
- `R/E/F/U`: instantaneous bottom phases: battlefield restore/base, enemy
  sprites, wall/bullets/effects and UI/profiler.
- `Q`: separation checks, target candidates and collision candidates.
- `ALIVE/TOTAL`: active and spawned sandbox entities.

`P` includes the two framebuffer DMA copies (`256x192` per screen). It is a
real DS presentation cost, while DeSmuME may add host-side overhead around the
same operation. Compare `S`, `T`, `B`, `R/E/F` before attributing a drop to
gameplay or sprites.

Required evidence for a performance change:

1. Capture the same stress scenario before and after the change.
2. Keep `ALIVE:384 TOTAL:384` in the comparison frames.
3. Run `scenarios/combat_fast_kill.json` as a gameplay regression.
4. Inspect the PNG HUD and `manifest.json`; a PASS without visible telemetry
   is insufficient.
