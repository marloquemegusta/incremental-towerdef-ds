# Known issues

## Sandbox visual validation

- The debug sandbox is integrated into `main` and can spawn enemies manually.
- The sandbox scenario has required fixes to its input/capture contract, but
  headless DeSmuME still does not reliably enter the sandbox through the
  automated hotkey/touch sequence. A successful visual capture with spawned
  enemies is still pending.
- Because of that, enemy facing and animation have not yet been confirmed
  visually on the DS with the sandbox workflow.

## Enemy sprites

- The Hydralisk direction sheet is normalized to the canonical order
  `N, NE, E, SE, S, SW, W, NW`.
- The small rotated enemy previously showed occasional vibration or apparent
  sprite corruption. Its rotation canvas is now fixed-size to avoid bounds
  changes and diagonal clipping, but this fix still needs confirmation on
  hardware.
- If the Hydralisk still walks backwards on the DS, the remaining suspect is
  the source sheet's front/back interpretation rather than the renderer's
  direction indexing.
