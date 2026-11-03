#!/usr/bin/env python3
"""Live preview of the gauge against the mock ECU, in your browser.

    python tools/preview.py            # builds the sim, serves http://localhost:8765
    python tools/preview.py --port 9000 --no-browser

The sim (`program --live`) streams raw 240x240 RGB frames on stdout; this script
turns each into a PNG and serves the newest one to preview.html, and forwards
button/slider commands to the sim's stdin (see lib/fakes/src/SimCommands.h).
"""
import argparse
import os
import shutil
import struct
import subprocess
import sys
import threading
import webbrowser
import zlib
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import parse_qs, urlparse

ROOT = Path(__file__).resolve().parent.parent
PAGE = ROOT / "tools" / "preview.html"
SIZE = 240
FRAME_BYTES = SIZE * SIZE * 3


def png_bytes(rgb: bytes) -> bytes:
    """Encode a SIZE x SIZE RGB frame as PNG (level-1 zlib: fast, plenty small)."""
    stride = SIZE * 3
    raw = b"".join(b"\x00" + rgb[y * stride:(y + 1) * stride] for y in range(SIZE))

    def chunk(kind: bytes, body: bytes) -> bytes:
        crc = zlib.crc32(kind + body) & 0xFFFFFFFF
        return struct.pack(">I", len(body)) + kind + body + struct.pack(">I", crc)

    header = struct.pack(">IIBBBBB", SIZE, SIZE, 8, 2, 0, 0, 0)
    return (b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", header)
            + chunk(b"IDAT", zlib.compress(raw, 1)) + chunk(b"IEND", b""))


class Frames:
    """Latest frame plus a sequence number; readers can wait for the next one."""

    def __init__(self):
        self.cond = threading.Condition()
        self.png = b""
        self.seq = 0

    def publish(self, png: bytes):
        with self.cond:
            self.png = png
            self.seq += 1
            self.cond.notify_all()

    def wait_after(self, seq: int, timeout: float = 2.0):
        with self.cond:
            self.cond.wait_for(lambda: self.seq > seq, timeout)
            return self.seq, self.png


def sim_path() -> Path:
    exe = "program.exe" if os.name == "nt" else "program"
    return ROOT / ".pio" / "build" / "sim" / exe


def pio_command():
    """The PlatformIO CLI, however it was installed (pip, pipx, VS Code extension)."""
    for name in ("pio", "platformio"):
        found = shutil.which(name)
        if found:
            return [found]
    vscode_pio = Path.home() / ".platformio" / "penv" / "Scripts" / "pio.exe"
    if vscode_pio.exists():
        return [str(vscode_pio)]
    return [sys.executable, "-m", "platformio"]


def env_with_compiler():
    """Environment for the build, with a C++ compiler on PATH if we can find one.

    A terminal opened before `winget install BrechtSanders.WinLibs...` does not
    have the new PATH, so look in WinGet's package folder as a fallback.
    """
    env = os.environ.copy()
    if shutil.which("g++", path=env.get("PATH", "")):
        return env
    packages = Path(env.get("LOCALAPPDATA", "")) / "Microsoft" / "WinGet" / "Packages"
    for bin_dir in packages.glob("BrechtSanders.WinLibs*/mingw64/bin"):
        if (bin_dir / "g++.exe").exists():
            env["PATH"] = str(bin_dir) + os.pathsep + env.get("PATH", "")
            print(f"g++ not on PATH; using {bin_dir}", flush=True)
            break
    return env


def build_sim():
    print("building sim ...", flush=True)
    result = subprocess.run(pio_command() + ["run", "-e", "sim"], cwd=ROOT,
                            capture_output=True, text=True, env=env_with_compiler())
    if result.returncode != 0:
        print(result.stdout + result.stderr, file=sys.stderr)
        sys.exit("sim build failed (see above). Is PlatformIO installed and is a C++ "
                 "compiler on PATH? See README step 1. Use --no-build to skip building.")


def read_frames(proc, frames: Frames, server):
    while True:
        buf = b""
        while len(buf) < FRAME_BYTES:
            part = proc.stdout.read(FRAME_BYTES - len(buf))
            if not part:
                print("sim exited", file=sys.stderr)
                server.shutdown()
                return
            buf += part
        frames.publish(png_bytes(buf))


def make_handler(frames: Frames, proc):
    stdin_lock = threading.Lock()

    class Handler(BaseHTTPRequestHandler):
        protocol_version = "HTTP/1.1"  # keep-alive: one connection for the whole stream

        def log_message(self, *args):  # keep the console quiet
            pass

        def _send(self, code, body=b"", ctype="text/plain", extra=None):
            self.send_response(code)
            self.send_header("Content-Type", ctype)
            self.send_header("Content-Length", str(len(body)))
            self.send_header("Cache-Control", "no-store")
            for key, value in (extra or {}).items():
                self.send_header(key, value)
            self.end_headers()
            self.wfile.write(body)

        def handle(self):
            try:
                super().handle()
            except ConnectionError:
                pass  # the browser tab closed or reloaded mid-request; not an error

        def do_GET(self):
            url = urlparse(self.path)
            if url.path == "/":
                self._send(200, PAGE.read_bytes(), "text/html; charset=utf-8")
            elif url.path == "/frame":
                after = int(parse_qs(url.query).get("after", ["0"])[0])
                seq, png = frames.wait_after(after)
                if seq <= after:
                    self._send(204)
                else:
                    self._send(200, png, "image/png", {"X-Seq": str(seq)})
            else:
                self._send(404)

        def do_POST(self):
            if urlparse(self.path).path != "/cmd":
                self._send(404)
                return
            length = int(self.headers.get("Content-Length", 0))
            command = self.rfile.read(length).decode("utf-8", "replace").strip()
            if not command or "\n" in command:
                self._send(400)
                return
            with stdin_lock:
                proc.stdin.write((command + "\n").encode())
                proc.stdin.flush()
            self._send(204)

    return Handler


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--port", type=int, default=8765)
    parser.add_argument("--no-browser", action="store_true")
    parser.add_argument("--no-build", action="store_true", help="reuse the existing sim binary")
    args = parser.parse_args()

    if not args.no_build:
        build_sim()
    # The sim links MinGW's runtime DLLs dynamically, so it needs the compiler's
    # bin directory on PATH to start, same as the build does.
    proc = subprocess.Popen([str(sim_path()), "--live"], stdin=subprocess.PIPE,
                            stdout=subprocess.PIPE, env=env_with_compiler())

    frames = Frames()
    server = ThreadingHTTPServer(("127.0.0.1", args.port), make_handler(frames, proc))
    threading.Thread(target=read_frames, args=(proc, frames, server), daemon=True).start()

    # 127.0.0.1, not "localhost": on Windows, localhost tries IPv6 first and each
    # request stalls waiting for the IPv4 fallback.
    url = f"http://127.0.0.1:{args.port}"
    print(f"preview: {url}   (Ctrl+C to stop)", flush=True)
    if not args.no_browser:
        webbrowser.open(url)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        proc.terminate()


if __name__ == "__main__":
    main()
