# Gauge design mockups

Source files for the four gauge concepts on the design canvas. Each `.dc.html` is one
artboard: an animated SVG mockup with a drive cycle, a hold-boost slider and a "Real size"
toggle (the true 32.5 mm display width).

| File | Concept |
| ---- | ------- |
| `Main.dc.html` | A: Modern Mini (chrome ring, black dial, orange needle) |
| `Retro.dc.html` | B: Retro 60s Cooper (cream dial, red needle). **Built as a real look** |
| `Minimal.dc.html` | C: Digital minimal (glowing arc, big readout) |
| `Motorsport.dc.html` | D: Motorsport (segmented heat bar, shift-light rim) |
| `canvas.json` | Canvas layout: artboard positions and sizes |

These are not standalone web pages. They need the Design canvas runtime (`support.js`),
so open them through the design canvas rather than directly in a browser.

The mockups reference the Cooper S badge by an uploaded-image id (`/_blob/...`). That
artwork is a trademark and is deliberately **not** in this repository, so those images
will not load from a copy of the repo. See the main README for how the badge is added
locally.

Design B's final artwork lives in `looks/retro/dial.svg`; see the README's "Looks" section.
