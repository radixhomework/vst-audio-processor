#include "controller.h"

#include "params.h"
#include "ui/editor.h"

#include "base/source/fstreamer.h"
#include "pluginterfaces/base/ustring.h"
#include "public.sdk/source/vst/vstparameters.h"

#include <array>
#include <cmath>
#include <cstring>
#include <format>
#include <iterator>

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
namespace {

using Steinberg::Vst::Parameter;
using Steinberg::Vst::ParamValue;

//------------------------------------------------------------------------
// Gain: 721 steps; index 0 = -inf (mute), then -60..+12 dB in 0.1 dB
// steps (audio-controls spec). Displayed in dB.
class GainParameter : public Parameter
{
public:
	explicit GainParameter (Steinberg::int32 flags)
	{
		Steinberg::UString (info.title, static_cast<Steinberg::int32> (std::size (info.title)))
		    .assign (USTRING ("Gain"));
		Steinberg::UString (info.units, static_cast<Steinberg::int32> (std::size (info.units)))
		    .assign (USTRING ("dB"));

		info.flags = flags;
		info.id = kGainId;
		info.stepCount = kGainStepCount;
		info.defaultNormalizedValue = gainDefaultNormalized ();
		info.unitId = Steinberg::Vst::kRootUnitId;
	}

	void toString (ParamValue value, Steinberg::Vst::String128 string) const SMTG_OVERRIDE
	{
		std::array<char, 32> text {};
		if (const int index = gainNormalizedToIndex (value); index == 0)
			std::format_to_n (text.data (), text.size () - 1, "-inf dB");
		else
			std::format_to_n (text.data (), text.size () - 1, "{:.1f} dB",
			                  gainIndexToDb (index));
		Steinberg::UString (string, 128).fromAscii (text.data ());
	}
};

//------------------------------------------------------------------------
// Pan: -100..+100, center default (audio-controls spec). Displayed as
// C / L<n> / R<n>.
class PanParameter : public Parameter
{
public:
	explicit PanParameter (Steinberg::int32 flags)
	{
		Steinberg::UString (info.title, static_cast<Steinberg::int32> (std::size (info.title)))
		    .assign (USTRING ("Pan"));

		info.flags = flags;
		info.id = kPanId;
		info.stepCount = 0;
		info.defaultNormalizedValue = panDefaultNormalized ();
		info.unitId = Steinberg::Vst::kRootUnitId;
	}

	void toString (ParamValue value, Steinberg::Vst::String128 string) const SMTG_OVERRIDE
	{
		const float pan = panFromNormalized (value);
		const auto offset = static_cast<int> (std::lround (std::fabs (pan) * 100.f));
		std::array<char, 16> text {};
		if (offset == 0)
			std::format_to_n (text.data (), text.size () - 1, "C");
		else if (pan < 0.f)
			std::format_to_n (text.data (), text.size () - 1, "L{}", offset);
		else
			std::format_to_n (text.data (), text.size () - 1, "R{}", offset);
		Steinberg::UString (string, 128).fromAscii (text.data ());
	}
};

} // namespace

//------------------------------------------------------------------------
Steinberg::tresult PLUGIN_API Controller::initialize (Steinberg::FUnknown* context)
{
	if (Steinberg::tresult result = EditController::initialize (context);
	    result != Steinberg::kResultTrue)
		return result;

	const Steinberg::int32 automate = Steinberg::Vst::ParameterInfo::kCanAutomate;

	// Audio controls (audio-controls spec, task 3.1).
	parameters.addParameter (new GainParameter (automate));
	parameters.addParameter (new PanParameter (automate));
	parameters.addParameter (STR16 ("Mono"), nullptr, 1, kMonoDefaultNormalized, automate, kMonoId);

	// Standard bypass (plugin-core spec): a two-step automatable parameter
	// flagged kIsBypass so hosts bind their own bypass control to it; the
	// processor applies it in process().
	parameters.addParameter (STR16 ("Bypass"), nullptr, 1, 0,
	                         automate | Steinberg::Vst::ParameterInfo::kIsBypass, kBypassId);

	return Steinberg::kResultTrue;
}

//------------------------------------------------------------------------
// Mirrors the processor's persisted state into the parameters so the host
// (its bypass control included) stays in sync after project load. The
// stream layout must match Processor::getState.
Steinberg::tresult PLUGIN_API Controller::setComponentState (Steinberg::IBStream* state)
{
	Steinberg::IBStreamer streamer (state, kLittleEndian);
	float savedBypass = 0.f;
	float savedGain = 0.f;
	float savedPan = 0.f;
	float savedMono = 0.f;
	if (!streamer.readFloat (savedBypass) || !streamer.readFloat (savedGain) ||
	    !streamer.readFloat (savedPan) || !streamer.readFloat (savedMono))
		return Steinberg::kResultFalse;

	setParamNormalized (kBypassId, savedBypass != 0.f ? 1. : 0.);
	setParamNormalized (kGainId, savedGain);
	setParamNormalized (kPanId, savedPan);
	setParamNormalized (kMonoId, savedMono != 0.f ? 1. : 0.);
	return Steinberg::kResultTrue;
}

//------------------------------------------------------------------------
Steinberg::tresult PLUGIN_API Controller::notify (Steinberg::Vst::IMessage* message)
{
	if (meterExchange.onMessage (message))
		return Steinberg::kResultTrue;
	return EditController::notify (message);
}

//------------------------------------------------------------------------
Steinberg::IPlugView* PLUGIN_API Controller::createView (Steinberg::FIDString name)
{
	if (name && Steinberg::FIDStringsEqual (name, Steinberg::Vst::ViewType::kEditor))
		return new Editor (this);
	return nullptr;
}

//------------------------------------------------------------------------
void PLUGIN_API Controller::queueOpened (Steinberg::Vst::DataExchangeUserContextID userContextID,
                                         Steinberg::uint32 /*blockSize*/,
                                         Steinberg::TBool& dispatchOnBackgroundThread)
{
	// Meter packets are tiny; handle them on the main thread.
	if (userContextID == kMeterQueueContextId)
		dispatchOnBackgroundThread = false;
}

//------------------------------------------------------------------------
void PLUGIN_API Controller::queueClosed (Steinberg::Vst::DataExchangeUserContextID /*userContextID*/)
{
}

//------------------------------------------------------------------------
void PLUGIN_API Controller::onDataExchangeBlocksReceived (
    Steinberg::Vst::DataExchangeUserContextID userContextID, Steinberg::uint32 numBlocks,
    Steinberg::Vst::DataExchangeBlock* block, Steinberg::TBool /*onBackgroundThread*/)
{
	if (userContextID != kMeterQueueContextId)
		return;
	for (Steinberg::uint32 i = 0; i < numBlocks; ++i)
	{
		if (block[i].size < sizeof (MeterPacket))
			continue;
		const auto& packet = *static_cast<const MeterPacket*> (block[i].data);
		for (int ch = 0; ch < kMaxMeterChannels; ++ch)
		{
			meterStateCache.setRms (ch, packet.rms[ch]);
			meterStateCache.setPeak (ch, packet.peak[ch]);
			meterStateCache.setClip (ch, (packet.clipFlags & (1u << ch)) != 0);
		}
	}
}

//------------------------------------------------------------------------
void Controller::requestClipReset (int channel)
{
	meterStateCache.clearClip (channel);
	if (auto* peer = getPeer ())
	{
		if (auto message = Steinberg::owned (allocateMessage ()))
		{
			message->setMessageID ("Meter/ResetClip");
			peer->notify (message);
		}
	}
}

} // namespace Radixhomework
