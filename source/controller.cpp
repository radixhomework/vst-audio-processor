#include "controller.h"

namespace Radixhomework {

//------------------------------------------------------------------------
Steinberg::FUnknown* Controller::createInstance (void*)
{
	return (Steinberg::Vst::IEditController*)new Controller ();
}

} // namespace Radixhomework
