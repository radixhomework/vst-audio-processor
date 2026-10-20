//------------------------------------------------------------------------
// Edit controller — placeholder implementation for the identity/factory
// milestone (task 1.2). Parameters land with task 3.1, the editor with 5.x.
//------------------------------------------------------------------------
#pragma once

#include "public.sdk/source/vst/vsteditcontroller.h"

namespace Radixhomework {

class Controller : public Steinberg::Vst::EditController
{
public:
	static Steinberg::FUnknown* createInstance (void*);
};

} // namespace Radixhomework
