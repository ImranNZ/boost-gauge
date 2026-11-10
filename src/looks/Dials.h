#pragma once

// Generated dial pictures for this build's display profile. The same FONT_PROFILE_* flag selects
// fonts and dial artwork, so the simulator and firmware cannot accidentally use different profiles.

#if defined(FONT_PROFILE_ALL)
#include "looks/modern/all/ModernDial.h"
#include "looks/retro/all/RetroDial.h"
#elif defined(FONT_PROFILE_P4_720)
#include "looks/modern/p4-720/ModernDial.h"
#include "looks/retro/p4-720/RetroDial.h"
#else
#include "looks/modern/ModernDial.h"
#include "looks/retro/RetroDial.h"
#endif
