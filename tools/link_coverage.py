"""PlatformIO extra script: link with gcov runtime for the coverage env."""
Import("env")  # noqa: F821  (provided by PlatformIO/SCons)

env.Append(LINKFLAGS=["--coverage"])  # noqa: F821
