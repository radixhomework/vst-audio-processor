#include "controller.h"

#include "params.h"

#include "base/source/fstreamer.h"

namespace Radixhomework {

//------------------------------------------------------------------------
// The VST3 factory contract requires this exact create-function signature
// (raw void* context) and transfers ownership to the host through the
// returned raw pointer.
Steinberg::FUnknown* Controller::createInstance (void* /*context*/) // NOSONAR
{
	return (Steinberg::Vst::IEditController*)new Controller (); // NOSONAR
}

//------------------------------------------------------------------------
Steinberg::tresult PLUGIN_API Controller::initialize (Steinberg::FUnknown* context)
{
	if (Steinberg::tresult result = EditController::initialize (context);
	    result != Steinberg::kResultTrue)
		return result;

	// Standard bypass (plugin-core spec): a two-step automatable parameter
	// flagged kIsBypass so hosts bind their own bypass control to it; the
	// processor applies it in process().
	const Steinberg::int32 stepCount = 1;
	const Steinberg::Vst::ParamValue defaultNormalizedValue = 0;
	const Steinberg::int32 flags =
	    Steinberg::Vst::ParameterInfo::kCanAutomate | Steinberg::Vst::ParameterInfo::kIsBypass;
	parameters.addParameter (STR16 ("Bypass"), nullptr, stepCount, defaultNormalizedValue, flags,
	                         kBypassId);

	return Steinberg::kResultTrue;
}

//------------------------------------------------------------------------
// Mirrors the processor's persisted state into the parameters so the host
// (its bypass control included) stays in sync after project load. The
// stream layout must match Processor::getState; audio parameters join in
// task 3.x.
Steinberg::tresult PLUGIN_API Controller::setComponentState (Steinberg::IBStream* state)
{
	Steinberg::IBStreamer streamer (state, kLittleEndian);
	float savedBypass = 0.f;
	if (!streamer.readFloat (savedBypass))
		return Steinberg::kResultFalse;

	setParamNormalized (kBypassId, savedBypass != 0.f ? 1. : 0.);
	return Steinberg::kResultTrue;
}

} // namespace Radixhomework
