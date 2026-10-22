//------------------------------------------------------------------------
// Audio processor — host contract: mono/stereo buses, 32/64-bit float
// processing, VST3 bypass (plugin-core spec) and the Round-1 chain
// mono-sum -> gain -> pan (audio-controls spec). All chain state lives
// in DspChain and is reset on activation; the audio path never
// allocates or blocks.
//------------------------------------------------------------------------
#pragma once

#include "dsp/chain.h"
#include "dsp/meter_state.h"
#include "params.h"

#include "pluginterfaces/vst/ivstdataexchange.h"
#include "public.sdk/source/vst/utility/dataexchange.h"
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
	Steinberg::tresult PLUGIN_API connect (Steinberg::Vst::IConnectionPoint* other) SMTG_OVERRIDE;
	Steinberg::tresult PLUGIN_API disconnect (Steinberg::Vst::IConnectionPoint* other) SMTG_OVERRIDE;
	Steinberg::tresult PLUGIN_API notify (Steinberg::Vst::IMessage* message) SMTG_OVERRIDE;
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
	// Reads the parameter queues for this block (bypass, gain, pan, mono);
	// runs even for zero-sample flush calls.
	void updateParameters (Steinberg::Vst::ProcessData& data);

	DspChain chain;
	Metering metering;

	// Post-pan output levels (bypass included), published to the controller
	// through the VST3 data exchange (design D5/D6).
	MeterState meterState;
	Steinberg::Vst::DataExchangeHandler meterExchange {
		this, [] (Steinberg::Vst::DataExchangeHandler::Config& config,
		          const Steinberg::Vst::ProcessSetup&) {
			config.blockSize = sizeof (MeterPacket);
			config.numBlocks = 16;
			config.userContextID = kMeterQueueContextId;
			return true;
		}};

	bool bypassActive {false};
	bool monoSumActive {false};

	// Last normalized values seen (or restored), for state persistence.
	Steinberg::Vst::ParamValue lastGainNormalized {gainDefaultNormalized ()};
	Steinberg::Vst::ParamValue lastPanNormalized {panDefaultNormalized ()};
	Steinberg::Vst::ParamValue lastMonoNormalized {kMonoDefaultNormalized};
};

} // namespace Radixhomework
