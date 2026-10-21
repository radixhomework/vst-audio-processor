//------------------------------------------------------------------------
// Edit controller — parameter registration (bypass for now; gain/pan/mono
// land with task 3.1) and later the editor (tasks 5.x).
//------------------------------------------------------------------------
#pragma once

#include "public.sdk/source/vst/vsteditcontroller.h"

namespace Radixhomework {

class Controller : public Steinberg::Vst::EditController
{
public:
	static Steinberg::FUnknown* createInstance (void* /*context*/);

	Steinberg::tresult PLUGIN_API initialize (Steinberg::FUnknown* context) SMTG_OVERRIDE;
	Steinberg::tresult PLUGIN_API setComponentState (Steinberg::IBStream* state) SMTG_OVERRIDE;
};

} // namespace Radixhomework
