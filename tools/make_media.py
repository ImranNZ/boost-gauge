#!/usr/bin/env python3
"""Render the README media: a still and an animation (WebP) of every look, plus a montage.

  python tools/make_media.py              # builds the sim, writes docs/media/
  python tools/make_media.py --no-build   # reuse the last build

It drives the same simulator as the live preview (the real gauge code against the mock
ECU) through its drive cycle and records the frames. The `sim-media` build leaves out
the Cooper S badge: it is brand artwork and the repo is public, so it must never appear
in anything committed.

Needs:  pip install pillow
"""
import argparse
import subprocess
import sys
from pathlib import Path

from PIL import Image, ImageDraw

sys.path.insert(0, str(Path(__file__).resolve().parent))
from preview import ROOT, env_with_compiler, pio_command  # noqa: E402

SIZE = 240
FRAME_BYTES = SIZE * SIZE * 3
FRAME_MS = 33          # the sim advances 33 ms per frame
LOOKS = ["modern", "retro", "minimal", "motorsport", "classic"]
CLIP_FROM_S, CLIP_TO_S = 2.0, 12.0   # idle -> pull -> hold -> lift-off of the drive cycle
STILL_AT_S = 7.0                     # near full boost
FRAME_STEP = 2                       # keep every 2nd frame: ~15 fps
WEBP_QUALITY = 80


def sim_exe() -> Path:
    name = "program.exe" if sys.platform == "win32" else "program"
    return ROOT / ".pio" / "build" / "sim-media" / name


def build():
    print("building sim-media ...", flush=True)
    result = subprocess.run(pio_command() + ["run", "-e", "sim-media"], cwd=ROOT,
                            capture_output=True, text=True, env=env_with_compiler())
    if result.returncode != 0:
        print(result.stdout + result.stderr, file=sys.stderr)
        sys.exit("sim-media build failed (see above)")


def capture(look: str) -> list[Image.Image]:
    """Frames from sim time 0 up to CLIP_TO_S, as RGB images."""
    count = int(CLIP_TO_S * 1000 / FRAME_MS) + 1
    proc = subprocess.Popen([str(sim_exe()), "--live", "--look", look], stdout=subprocess.PIPE,
                            stderr=subprocess.DEVNULL, env=env_with_compiler())
    frames = []
    try:
        for _ in range(count):
            data = proc.stdout.read(FRAME_BYTES)
            if len(data) < FRAME_BYTES:
                sys.exit(f"{look}: the sim ended early")
            frames.append(Image.frombytes("RGB", (SIZE, SIZE), data))
    finally:
        proc.kill()
        proc.wait()
    return frames


def circle_mask(scale: int = 8) -> Image.Image:
    """The round display as an anti-aliased alpha mask."""
    big = Image.new("L", (SIZE * scale, SIZE * scale), 0)
    ImageDraw.Draw(big).ellipse((0, 0, SIZE * scale - 1, SIZE * scale - 1), fill=255)
    return big.resize((SIZE, SIZE), Image.LANCZOS)


def with_round_corners(frame: Image.Image, mask: Image.Image) -> Image.Image:
    rgba = frame.convert("RGBA")
    rgba.putalpha(mask)
    return rgba


def save_animation(frames: list[Image.Image], path: Path, mask: Image.Image):
    """Animated WebP: small, and the round corners stay properly anti-aliased (a GIF
    would be several times larger on the picture dials and have jagged edges)."""
    rgba = [with_round_corners(frame, mask) for frame in frames]
    rgba[0].save(path, save_all=True, append_images=rgba[1:], duration=FRAME_MS * FRAME_STEP, loop=0,
                 quality=WEBP_QUALITY, method=6)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--no-build", action="store_true", help="reuse the last sim-media build")
    parser.add_argument("--out", type=Path, default=ROOT / "docs" / "media")
    args = parser.parse_args()

    if not args.no_build:
        build()
    args.out.mkdir(parents=True, exist_ok=True)
    mask = circle_mask()

    stills = []
    for look in LOOKS:
        print(f"recording {look} ...", flush=True)
        frames = capture(look)
        first = int(CLIP_FROM_S * 1000 / FRAME_MS)
        clip = frames[first::FRAME_STEP]

        still = with_round_corners(frames[int(STILL_AT_S * 1000 / FRAME_MS)], mask)
        still.save(args.out / f"{look}.png", optimize=True)
        stills.append(still)

        save_animation(clip, args.out / f"{look}.webp", mask)
        size_kb = (args.out / f"{look}.webp").stat().st_size // 1024
        print(f"  {look}.png, {look}.webp ({len(clip)} frames, {size_kb} KB)")

    gap = 16
    montage = Image.new("RGBA", (SIZE * len(stills) + gap * (len(stills) - 1), SIZE), (0, 0, 0, 0))
    for i, still in enumerate(stills):
        montage.paste(still, (i * (SIZE + gap), 0))
    montage.save(args.out / "looks.png", optimize=True)
    print(f"wrote {args.out / 'looks.png'}")


if __name__ == "__main__":
    main()
