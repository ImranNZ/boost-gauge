#!/usr/bin/env python3
"""Visual regression test: render every look at fixed moments and compare with stored images.

  python tools/check_renders.py             # compare against test/golden/ (builds the sim)
  python tools/check_renders.py --update    # re-record test/golden/ after an intended change
  python tools/check_renders.py --no-build  # reuse the last sim-media build

It drives the simulator through the drive cycle (the real gauge code against the mock ECU)
and grabs the frames at a few instants. The frames depend only on simulated time, so they
are the same run to run. A small tolerance absorbs floating-point differences between
compilers and platforms; a real change (a moved needle, a different font, a shifted
label) alters hundreds of pixels and fails. On a failure the got/want/diff images are
written to out/render-diff/ so you can see what moved.

Uses the sim-media build, which leaves the Cooper S badge out: the stored images are
committed and the badge is brand artwork.

Needs:  pip install pillow
"""
import argparse
import subprocess
import sys
from pathlib import Path

from PIL import Image, ImageChops

sys.path.insert(0, str(Path(__file__).resolve().parent))
from make_media import FRAME_MS, build, sim_exe  # noqa: E402
from preview import ROOT, env_with_compiler  # noqa: E402

# (look, channel, unit, screen, menu): every look on boost in psi, then the code-drawn looks on the other
# channels and units, then a few on the other screen sizes (466x466 and 412x412 are round AMOLED/LCD
# boards, 320x480 a portrait one), then the settings menu open on each of its choices. Retro and Modern
# are pictures of a psi boost dial, so only boost.
CASES = [(look, "boost", "", (240, 240), "") for look in ("modern", "retro", "minimal", "motorsport", "classic")] + [
    ("classic", "boost", "bar", (240, 240), ""),
    ("minimal", "coolant", "", (240, 240), ""),
    ("motorsport", "coolant", "f", (240, 240), ""),
    ("motorsport", "intake", "", (240, 240), ""),
    ("classic", "voltage", "", (240, 240), ""),
    ("classic", "boost", "", (466, 466), ""),
    ("minimal", "boost", "", (466, 466), ""),
    ("retro", "boost", "", (466, 466), ""),
    ("motorsport", "coolant", "", (412, 412), ""),
    ("minimal", "boost", "", (320, 480), ""),
    ("retro", "boost", "", (240, 240), "look"),
    ("minimal", "coolant", "", (240, 240), "gauge"),
    ("classic", "boost", "bar", (466, 466), "unit"),
]
# Seconds into the drive cycle: idle in vacuum, near full boost, just after lifting off.
MOMENTS_S = [1.0, 6.0, 10.5]

TOLERANCE = 16        # a channel may differ by this much (anti-aliasing / rounding) ...
MAX_BAD_PIXELS = 40   # ... and at most this many pixels may differ by more (of 57,600)

GOLDEN = ROOT / "test" / "golden"
DIFF_DIR = ROOT / "out" / "render-diff"


def case_name(look: str, channel: str, unit: str, screen: tuple[int, int], menu: str = "") -> str:
    """The stored images' stem; boost in psi on 240x240 keeps the plain look name."""
    size = "" if screen == (240, 240) else f"{screen[0]}x{screen[1]}"
    parts = (look, "" if channel == "boost" else channel, unit, size, f"menu-{menu}" if menu else "")
    return "_".join(part for part in parts if part)


def frames_at(look: str, channel: str, unit: str, screen: tuple[int, int],
              menu: str = "") -> dict[float, Image.Image]:
    """The frames of one look at each moment, rendered flat out (no real-time pacing)."""
    wanted = {round(t * 1000 / FRAME_MS): t for t in MOMENTS_S}
    last = max(wanted)
    width, height = screen
    frame_bytes = width * height * 3
    args = ["--live", "--no-pace", "--channel", channel, "--look", look, "--size", f"{width}x{height}"]
    if unit:
        args += ["--unit", unit]
    if menu:
        args += ["--menu", menu]
    proc = subprocess.Popen([str(sim_exe())] + args,
                            stdout=subprocess.PIPE, stderr=subprocess.DEVNULL, env=env_with_compiler())
    found = {}
    try:
        for index in range(last + 1):
            data = proc.stdout.read(frame_bytes)
            if len(data) < frame_bytes:
                sys.exit(f"{case_name(look, channel, unit, screen, menu)}: the sim ended early")
            if index in wanted:
                found[wanted[index]] = Image.frombytes("RGB", (width, height), data)
    finally:
        proc.kill()
        proc.wait()
    return found


def bad_pixels(got: Image.Image, want: Image.Image) -> tuple[int, Image.Image]:
    """How many pixels differ by more than TOLERANCE in any channel, and a diff image."""
    diff = ImageChops.difference(got, want)
    r, g, b = diff.split()
    worst = ImageChops.lighter(ImageChops.lighter(r, g), b)
    over = worst.point(lambda v: 255 if v > TOLERANCE else 0)
    return over.histogram()[255], over


def name_of(case: str, moment: float) -> str:
    return f"{case}_{moment:04.1f}s.png"


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--update", action="store_true", help="re-record the stored images")
    parser.add_argument("--no-build", action="store_true", help="reuse the last sim-media build")
    args = parser.parse_args()

    if not args.no_build:
        build()
    GOLDEN.mkdir(parents=True, exist_ok=True)

    failures = []
    for look, channel, unit, screen, menu in CASES:
        case = case_name(look, channel, unit, screen, menu)
        for moment, got in frames_at(look, channel, unit, screen, menu).items():
            path = GOLDEN / name_of(case, moment)
            if args.update:
                got.save(path, optimize=True)
                continue
            if not path.exists():
                failures.append(f"{path.name}: no stored image (run with --update)")
                continue
            want = Image.open(path).convert("RGB")
            bad, over = bad_pixels(got, want)
            status = "ok" if bad <= MAX_BAD_PIXELS else "CHANGED"
            print(f"{path.name:28} {bad:5d} pixels over tolerance  {status}")
            if bad > MAX_BAD_PIXELS:
                DIFF_DIR.mkdir(parents=True, exist_ok=True)
                stem = path.stem
                got.save(DIFF_DIR / f"{stem}_got.png")
                want.save(DIFF_DIR / f"{stem}_want.png")
                over.save(DIFF_DIR / f"{stem}_diff.png")
                failures.append(f"{path.name}: {bad} pixels differ (see out/render-diff/)")

    if args.update:
        print(f"re-recorded {len(CASES) * len(MOMENTS_S)} images in {GOLDEN.relative_to(ROOT)}")
        return 0
    if failures:
        print("\nrender check failed:\n  " + "\n  ".join(failures), file=sys.stderr)
        print("\nIf the change is intended, run: python tools/check_renders.py --update "
              "and commit test/golden/.", file=sys.stderr)
        return 1
    print(f"all {len(CASES) * len(MOMENTS_S)} renders match")
    return 0


if __name__ == "__main__":
    sys.exit(main())
