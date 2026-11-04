#pragma once

namespace gauge {

/// The geometry a pre-rendered dial was drawn with. It is generated from the look's
/// spec.json (see tools/make_look.py), the single source for both the picture and the
/// code, so the pointer always lines up with the printed scale.
struct PointerSpec {
  float minPsi;           // scale the dial is graduated for
  float maxPsi;
  float needleLength;     // pixels from the pivot to the tip
  float tailLength;       // pixels from the pivot to the end of the tail
  float flagInnerRadius;  // the peak flag sits between these two radii
  float flagOuterRadius;
  int hubRadius;
};

}  // namespace gauge
