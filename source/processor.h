//------------------------------------------------------------------------
// Audio processor — placeholder implementation for the identity/factory
// milestone (task 1.2). Bus contract, bypass and the DSP chain land with
// tasks 2.x and 3.x.
//------------------------------------------------------------------------
#pragma once

#include "public.sdk/source/vst/vstaudioeffect.h"

namespace Radixhomework {

// Deriving from AudioEffect (the SDK's standard processor base) is what
// fixes the depth of the inheritance hierarchy below the SDK's own layers.
class Processor : public Steinberg::Vst::AudioEffect // NOSONAR
{
public:
	Processor ();

	static Steinberg::FUnknown* createInstance (void*);

	Steinberg::tresult PLUGIN_API initialize (Steinberg::FUnknown* context) SMTG_OVERRIDE;
	Steinberg::tresult PLUGIN_API process (Steinberg::Vst::ProcessData& data) SMTG_OVERRIDE;
	Steinberg::tresult PLUGIN_API setState (Steinberg::IBStream* state) SMTG_OVERRIDE;
	Steinberg::tresult PLUGIN_API getState (Steinberg::IBStream* state) SMTG_OVERRIDE;
};

} // namespace Radixhomework
