//------------------------------------------------------------------------
// Parameter IDs — single source of truth (design D3), shared by the edit
// controller (registration) and the processor (queue handling).
//
// IDs are stable once released: hosts store them inside automation data.
//------------------------------------------------------------------------
#pragma once

#include "pluginterfaces/vst/vsttypes.h"

namespace Radixhomework {

constexpr Steinberg::Vst::ParamID kBypassId = 1; ///< Standard bypass (kIsBypass).

} // namespace Radixhomework
