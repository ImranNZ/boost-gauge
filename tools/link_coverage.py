"""PlatformIO extra script: link with gcov runtime for the coverage env.

Set GAUGE_GCC_VERSION (tools/coverage.sh does this on macOS) to build with a
versioned GCC such as g++-14 instead of the default `g++`, which on macOS is clang.
"""
import os

Import("env")  # noqa: F821  (provided by PlatformIO/SCons)

version = os.environ.get("GAUGE_GCC_VERSION")
if version:
    env.Replace(  # noqa: F821
        CC=f"gcc-{version}", CXX=f"g++-{version}", LINK=f"g++-{version}")

env.Append(LINKFLAGS=["--coverage"])  # noqa: F821
