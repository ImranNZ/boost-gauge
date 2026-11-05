---
name: add-look
description: Add a new visual style ("look") to the boost gauge, such as a new dial design, with its artwork, sim preview entry and tests. Use when the user wants another gauge design or theme.
---

# Add a look

A look is an `ILook` (`lib/core/src/looks/ILook.h`) with two methods: `drawDial` (the static
artwork; `FrameRenderer` runs it once and the canvas caches it) and `drawDynamic` (the
moving parts, every frame). A look draws only through `IDrawSurface`; it never sees the
cache or `present`. Study `RetroLook` (picture dial) and `ClassicLook` / `MinimalLook` (drawn in code; Minimal is the simpler pattern if the design is code-drawn, and shows the optional `Badge`) first.

1. **Artwork + spec.** Static layer only: no needle, readout or live text. Put the scale and
   pointer geometry (range, tick radii, needle length, hub size) in `looks/<name>/spec.json`
   and author `looks/<name>/dial.svg.in`, 240x240 (the design space: the renderer scales a look to
   the real screen, see `ScaledSurface`), with placeholders for anything computed
   from the spec (see `looks/retro/`). Fonts are static TTFs under `looks/<name>/fonts/`
   (SIL OFL only; ship the licence). Keep it free of trademarked artwork. For code-drawn text in a real typeface, add the TTF
   (SIL OFL) under `looks/<name>/fonts/`, list it in `tools/fonts.json`, run
   `python tools/make_font.py`, and pass the resulting `Font`s to the look (see `MinimalLook`).
2. **Generate.** Add a `<name>` command to `tools/make_look.py` (copy `retro`) that writes
   `src/looks/<name>/<Name>Dial.h` and `<Name>Spec.h` from the spec. The picture and the
   code must read the same numbers: never hard-code geometry in the look class. Commit the
   generated headers (never edit them by hand) and look at `out/dial_render.png`.
3. **Look class** in `lib/core/src/looks/<Name>Look.h/.cpp`, `namespace gauge`, no hardware
   headers. Use only `IDrawSurface` primitives so the PC sim and the board match. Needles
   are `drawWedge` with float coordinates; pictures go through `drawImage`. Use
   `GaugeScale` for pressure -> angle.
   A code-drawn look takes a `GaugeProfile` (`gauge/GaugeProfile.h`) and uses it for the scale
   range, zone thresholds, unit label and number formatting instead of hard-coding psi numbers (see
   `MinimalLook`); convert `state.psi` with `profile.toDisplay()` once, then work in that unit.
4. **Factory** in `src/looks/<name>/<Name>LookFactory.h` that wraps the generated arrays in
   `ImageView`s and fills the look's `Spec`, shared by `src/main.cpp` and the sim. Add
   `static_assert`s for anything the C++ also hard-codes (see `src/looks/retro/RetroLookFactory.h`).
5. **Register it:**
   - sim: add the name to `Rig::selectLook` (`src/sim/Rig.h`) and an `<option>` in
     `tools/preview.html`;
   - firmware: add it to `config::Look` in `src/config.h` and to `chosenLook()` in `src/main.cpp`.
6. **Tests** (`test/test_core/test_retro_look.cpp` has the pattern, helpers in
   `test_support.h`): the needle lands at the right angle for several pressures, NO LINK,
   the peak marker, the fps label, every optional part. Dial caching is `FrameRenderer`'s
   job and is already tested; do not retest it per look. Keep coverage at 100%.
7. **Verify visually:** `python tools/preview.py`, switch to the look, check the drive
   cycle and Real size. Then `pio run -e device-demo` to be sure it compiles for the board.

Brand artwork rule: logos or badges go under `assets/` and `src/looks/local/`
(both git-ignored) and are included only through `__has_include`. Never commit them.
