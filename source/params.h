//------------------------------------------------------------------------
// Parameter IDs — single source of truth (design D3), shared by the edit
// controller (registration) and the processor (queue handling).
//
// IDs are stable once released: hosts store them inside automation data.
//------------------------------------------------------------------------
#pragma once

namespace Radixhomework {

enum ParamIds
{
	kBypassId = 1, ///< Standard bypass (kIsBypass flag); applied by the processor.
};

} // namespace Radixhomework
