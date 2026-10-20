#include "controller.h"

namespace Radixhomework {

//------------------------------------------------------------------------
// The VST3 factory contract requires this exact create-function signature
// (raw void* context) and transfers ownership to the host through the
// returned raw pointer.
Steinberg::FUnknown* Controller::createInstance (void* /*context*/) // NOSONAR
{
	return (Steinberg::Vst::IEditController*)new Controller (); // NOSONAR
}

} // namespace Radixhomework
