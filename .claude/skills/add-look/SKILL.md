---
name: add-look
description: Add a new visual style ("look") to the boost gauge, such as a new dial design, with its artwork, sim preview entry and tests. Use when the user wants another gauge design or theme.
---

# Add a look

A look is an `ILook` that draws the gauge for a given `GaugeState`. Most looks are a
static dial plus a few moving parts, so derive from `DialCachingLook`
(`lib/core/src/ILook.h`): implement `drawDial` (runs once; the canvas caches it) and
`drawDynamic` (every frame). Study `RetroLook` (picture dial) and `GaugeView` (dial drawn
in code) first.

1. **Artwork.** Static layer only: no needle, readout or live text. Author it as an SVG
   in `looks/<name>/dial.svg`, 240x240, with fonts as static TTFs under
   `looks/<name>/fonts/` (SIL OFL fonts only; ship the licence). Keep it free of
   trademarked artwork; brand marks are drawn from a local, git-ignored file.
2. **Generate.** `python tools/make_look.py dial looks/<name>/dial.svg src/looks/<Name>Dial.h`.
   Commit the generated header (never edit it by hand). Look at `out/dial_render.png`.
3. **Look class** in `lib/core/src/<Name>Look.h/.cpp`, `namespace gauge`, no hardware
   headers. Use only `ICanvas` primitives so the PC sim and the board match. Needles
   are `drawWedge` with float coordinates; pictures go through `drawImage`. Use
   `GaugeScale` for pressure -> angle.
4. **Factory** in `src/looks/<Name>LookFactory.h` that wraps the generated arrays in
   `ImageView`s, shared by `src/main.cpp` and the sim.
5. **Register it:**
   - sim: add the name to `Rig::selectLook` (`src/sim/Rig.h`) and an `<option>` in
     `tools/preview.html`;
   - firmware: extend the choice in `src/config.h` / `src/main.cpp`.
6. **Tests** (`test/test_core/test_looks.cpp` has the pattern): it draws the dial once
   then restores it, the needle lands at the right angle for several pressures, NO LINK,
   the peak marker, the fps label, every optional part. Keep coverage at 100%.
7. **Verify visually:** `python tools/preview.py`, switch to the look, check the drive
   cycle and Real size. Then `pio run -e device-demo` to be sure it compiles for the board.

Brand artwork rule: logos or badges go under `assets/` and `src/looks/local/`
(both git-ignored) and are included only through `__has_include`. Never commit them.
