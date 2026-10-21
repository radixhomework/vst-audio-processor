//------------------------------------------------------------------------
// Audio processor — host contract: mono/stereo buses, 32/64-bit float
// processing, VST3 bypass (plugin-core spec). The audible chain
// (audio-controls spec) lands with tasks 3.x.
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

	static Steinberg::FUnknown* createInstance (void* /*context*/);

	Steinberg::tresult PLUGIN_API initialize (Steinberg::FUnknown* context) SMTG_OVERRIDE;
	Steinberg::tresult PLUGIN_API setBusArrangements (Steinberg::Vst::SpeakerArrangement* inputs,
	                                                  Steinberg::int32 numIns,
	                                                  Steinberg::Vst::SpeakerArrangement* outputs,
	                                                  Steinberg::int32 numOuts) SMTG_OVERRIDE;
	Steinberg::tresult PLUGIN_API canProcessSampleSize (Steinberg::int32 symbolicSampleSize) SMTG_OVERRIDE;
	Steinberg::tresult PLUGIN_API setActive (Steinberg::TBool state) SMTG_OVERRIDE;
	Steinberg::tresult PLUGIN_API process (Steinberg::Vst::ProcessData& data) SMTG_OVERRIDE;
	Steinberg::tresult PLUGIN_API setState (Steinberg::IBStream* state) SMTG_OVERRIDE;
	Steinberg::tresult PLUGIN_API getState (Steinberg::IBStream* state) SMTG_OVERRIDE;

private:
	// Reads the bypass parameter queue for this block into bypassActive.
	void updateBypassState (Steinberg::Vst::ProcessData& data);

	bool bypassActive {false};
};

} // namespace Radixhomework
