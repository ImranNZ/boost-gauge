#pragma once

// The typeface tables for this build's display class. Each profile is generated from
// tools/fonts.json by `python tools/make_font.py --profile <name>` into
// src/looks/fonts/<name>/; this header picks the one the environment asks for.
//
// A profile names the resolutions (real pixels per design pixel) it has faces for. The build
// asserts in src/main.cpp that the selected profile covers the board's screen, so a board cannot
// silently render text that has been upscaled from a smaller face. A new display class is a new
// profile in fonts.json, a generated directory, and a branch here.

#if defined(FONT_PROFILE_P4_720)
#include "fonts/p4-720/Index.h"
#elif defined(FONT_PROFILE_ALL)
#include "fonts/all/Index.h"
#else
#include "fonts/s3/Index.h"
#endif
