#include "processor.h"

#include "params.h"
#include "version.h"

#include "pluginterfaces/vst/ivstparameterchanges.h"

#include "base/source/fstreamer.h"

#include <algorithm>
#include <cstring>

namespace Radixhomework {

//------------------------------------------------------------------------
Processor::Processor ()
{
	// Tells the host which controller class belongs to this processor.
	setControllerClass (audioProcessorControllerUID);
}

//------------------------------------------------------------------------
// The VST3 factory contract requires this exact create-function signature
// (raw void* context) and transfers ownership to the host through the
// returned raw pointer.
Steinberg::FUnknown* Processor::createInstance (void* /*context*/) // NOSONAR
{
	return (Steinberg::Vst::IAudioProcessor*)new Processor (); // NOSONAR
}

//------------------------------------------------------------------------
Steinberg::tresult PLUGIN_API Processor::initialize (Steinberg::FUnknown* context)
{
	if (Steinberg::tresult result = AudioEffect::initialize (context);
	    result != Steinberg::kResultTrue)
		return result;

	// Default arrangement is stereo; hosts request mono on mono tracks via
	// setBusArrangements (accepted there).
	addAudioInput (STR16 ("Audio Input"), Steinberg::Vst::SpeakerArr::kStereo);
	addAudioOutput (STR16 ("Audio Output"), Steinberg::Vst::SpeakerArr::kStereo);
	return Steinberg::kResultTrue;
}

//------------------------------------------------------------------------
// Connection: the meter stream follows the processor-controller
// connection; blocks are only exchanged while both sides are connected.
Steinberg::tresult PLUGIN_API Processor::connect (Steinberg::Vst::IConnectionPoint* other)
{
	const Steinberg::tresult result = AudioEffect::connect (other);
	meterExchange.onConnect (other, getHostContext ());
	return result;
}

//------------------------------------------------------------------------
Steinberg::tresult PLUGIN_API Processor::disconnect (Steinberg::Vst::IConnectionPoint* other)
{
	meterExchange.onDisconnect (other);
	return AudioEffect::disconnect (other);
}

//------------------------------------------------------------------------
Steinberg::tresult PLUGIN_API Processor::notify (Steinberg::Vst::IMessage* message)
{
	if (message && message->getMessageID () &&
	    strcmp (message->getMessageID (), "Meter/ResetClip") == 0)
	{
		// UI clip-LED click: clear the latched flags (task 4.2 reset path).
		for (int ch = 0; ch < kMaxMeterChannels; ++ch)
			meterState.clearClip (ch);
		return Steinberg::kResultTrue;
	}
	return AudioEffect::notify (message);
}

//------------------------------------------------------------------------
// Bus contract (plugin-core spec): exactly mono 1-in/1-out or stereo
// 2-in/2-out. Anything else — other channel counts, mismatched in/out
// arrangements, or extra (sidechain) buses — is rejected, so hosts never
// offer those layouts.
Steinberg::tresult PLUGIN_API Processor::setBusArrangements (Steinberg::Vst::SpeakerArrangement* inputs,
                                                             Steinberg::int32 numIns,
                                                             Steinberg::Vst::SpeakerArrangement* outputs,
                                                             Steinberg::int32 numOuts)
{
	if (numIns != 1 || numOuts != 1)
		return Steinberg::kResultFalse;

	const Steinberg::Vst::SpeakerArrangement in = inputs[0];
	const Steinberg::Vst::SpeakerArrangement out = outputs[0];

	if (in != out || (in != Steinberg::Vst::SpeakerArr::kMono &&
	                  in != Steinberg::Vst::SpeakerArr::kStereo))
		return Steinberg::kResultFalse;

	getAudioInput (0)->setArrangement (in);
	getAudioOutput (0)->setArrangement (out);
	return Steinberg::kResultTrue;
}

//------------------------------------------------------------------------
// Both 32-bit and 64-bit float streams are processed (plugin-core spec).
Steinberg::tresult PLUGIN_API Processor::canProcessSampleSize (Steinberg::int32 symbolicSampleSize)
{
	return (symbolicSampleSize == Steinberg::Vst::kSample32 ||
	        symbolicSampleSize == Steinberg::Vst::kSample64)
	           ? Steinberg::kResultTrue
	           : Steinberg::kResultFalse;
}

//------------------------------------------------------------------------
// Activation fixes the chain's ramp length from the sample rate and
// settles the smoothers at the current targets, so the audio path never
// allocates or blocks (plugin-core spec, real-time safety).
Steinberg::tresult PLUGIN_API Processor::setActive (Steinberg::TBool state)
{
	if (state)
	{
		chain.prepare (processSetup.sampleRate, linearGainFromNormalized (lastGainNormalized),
		               panFromNormalized (lastPanNormalized), monoSumActive);
		metering.prepare (processSetup.sampleRate);
		meterExchange.onActivate (processSetup);
	}
	else
		meterExchange.onDeactivate ();
	return AudioEffect::setActive (state);
}

//------------------------------------------------------------------------
namespace {

void copyBus (const Steinberg::Vst::AudioBusBuffers& in, Steinberg::Vst::AudioBusBuffers& out,
              Steinberg::int32 numSamples, Steinberg::int32 sampleSize)
{
	const Steinberg::int32 numChannels = std::min (in.numChannels, out.numChannels);
	for (Steinberg::int32 ch = 0; ch < numChannels; ++ch)
	{
		if (sampleSize == Steinberg::Vst::kSample64)
		{
			if (in.channelBuffers64[ch] != out.channelBuffers64[ch])
				memcpy (out.channelBuffers64[ch], in.channelBuffers64[ch],
				        numSamples * sizeof (Steinberg::Vst::Sample64));
		}
		else if (in.channelBuffers32[ch] != out.channelBuffers32[ch])
		{
			memcpy (out.channelBuffers32[ch], in.channelBuffers32[ch],
			        numSamples * sizeof (Steinberg::Vst::Sample32));
		}
	}
	out.silenceFlags = 0;
}

//------------------------------------------------------------------------
void clearBus (Steinberg::Vst::AudioBusBuffers& out, Steinberg::int32 numSamples,
               Steinberg::int32 sampleSize)
{
	for (Steinberg::int32 ch = 0; ch < out.numChannels; ++ch)
	{
		if (sampleSize == Steinberg::Vst::kSample64 && out.channelBuffers64)
			memset (out.channelBuffers64[ch], 0, numSamples * sizeof (Steinberg::Vst::Sample64));
		else if (out.channelBuffers32)
			memset (out.channelBuffers32[ch], 0, numSamples * sizeof (Steinberg::Vst::Sample32));
	}
	out.silenceFlags = (out.numChannels >= 64) ? ~Steinberg::uint64 (0)
	                                           : ((Steinberg::uint64 (1) << out.numChannels) - 1);
}

//------------------------------------------------------------------------
// Strict input->output pass-through (unmodified samples), used by the
// bypass path. Emits silence when no input bus is connected.
void passThrough (Steinberg::Vst::ProcessData& data)
{
	if (data.numInputs > 0)
		copyBus (data.inputs[0], data.outputs[0], data.numSamples, data.symbolicSampleSize);
	else
		clearBus (data.outputs[0], data.numSamples, data.symbolicSampleSize);
}

} // namespace

//------------------------------------------------------------------------
// Host parameter changes: the last point of each queue wins; continuous
// parameters ramp in the chain (~15 ms), so per-block granularity is
// sufficient (design D4).
void Processor::updateParameters (Steinberg::Vst::ProcessData& data)
{
	Steinberg::Vst::IParameterChanges* paramChanges = data.inputParameterChanges;
	if (!paramChanges)
		return;

	for (Steinberg::int32 i = 0; i < paramChanges->getParameterCount (); ++i)
	{
		Steinberg::Vst::IParamValueQueue* queue = paramChanges->getParameterData (i);
		if (!queue)
			continue;

		Steinberg::Vst::ParamValue value = 0;
		Steinberg::int32 sampleOffset = 0;
		if (const Steinberg::int32 numPoints = queue->getPointCount ();
		    numPoints <= 0 ||
		    queue->getPoint (numPoints - 1, sampleOffset, value) != Steinberg::kResultTrue)
			continue;

		switch (queue->getParameterId ())
		{
			case kBypassId:
				bypassActive = (value != 0.);
				break;

			case kGainId:
				lastGainNormalized = value;
				chain.setGainTarget (linearGainFromNormalized (value));
				break;

			case kPanId:
				lastPanNormalized = value;
				chain.setPanTarget (panFromNormalized (value));
				break;

			case kMonoId:
				lastMonoNormalized = value;
				monoSumActive = (value != 0.);
				chain.setMonoSumEnabled (monoSumActive);
				break;

			default:
				break;
		}
	}
}

//------------------------------------------------------------------------
Steinberg::tresult PLUGIN_API Processor::process (Steinberg::Vst::ProcessData& data)
{
	// Parameter queues are handled even for zero-sample flush calls
	// (numSamples == 0, no buffers): hosts use those to advance parameter
	// state, and saved state must reflect it.
	updateParameters (data);

	if (data.numSamples <= 0 || data.numOutputs <= 0)
		return Steinberg::kResultTrue;

	if (data.symbolicSampleSize != Steinberg::Vst::kSample32 &&
	    data.symbolicSampleSize != Steinberg::Vst::kSample64)
		return Steinberg::kResultFalse;

	if (bypassActive)
	{
		// Bypass (plugin-core spec): input passed to the output unmodified;
		// gain/pan/mono cannot affect the audio while engaged. Metering
		// keeps measuring — the output is the input then.
		passThrough (data);
	}
	else
	{
		// Processing chain (audio-controls spec): work on the copied input
		// in the output buffers, then run the fixed-order stages. No input
		// bus connected -> the chain runs on (and keeps emitting) silence.
		if (data.numInputs > 0)
			copyBus (data.inputs[0], data.outputs[0], data.numSamples, data.symbolicSampleSize);
		else
			clearBus (data.outputs[0], data.numSamples, data.symbolicSampleSize);
		chain.process (data.outputs[0], data.numSamples, data.symbolicSampleSize);
	}

	// Meter tap: post-pan final output on every path (metering spec).
	metering.measure (data.outputs[0], data.numSamples, data.symbolicSampleSize, meterState);

	// Publish a meter snapshot to the controller (design D5); lock-free,
	// and a no-op unless the queue is open.
	if (auto block = meterExchange.getCurrentOrNewBlock ();
	    block.blockID != Steinberg::Vst::InvalidDataExchangeBlockID &&
	    block.size >= sizeof (MeterPacket))
	{
		auto& packet = *static_cast<MeterPacket*> (block.data);
		for (int ch = 0; ch < kMaxMeterChannels; ++ch)
		{
			packet.rms[ch] = meterState.getRms (ch);
			packet.peak[ch] = meterState.getPeak (ch);
			if (meterState.getClip (ch))
				packet.clipFlags |= (1u << ch);
			else
				packet.clipFlags &= ~(1u << ch);
		}
		meterExchange.sendCurrentBlock ();
	}
	return Steinberg::kResultTrue;
}

//------------------------------------------------------------------------
// Persistent state: bypass, gain, pan, mono as normalized floats. The
// layout is shared with Controller::setComponentState; task 3.x features
// extend it here first.
Steinberg::tresult PLUGIN_API Processor::setState (Steinberg::IBStream* state)
{
	Steinberg::IBStreamer streamer (state, kLittleEndian);
	float savedBypass = 0.f;
	float savedGain = 0.f;
	float savedPan = 0.f;
	float savedMono = 0.f;
	if (!streamer.readFloat (savedBypass) || !streamer.readFloat (savedGain) ||
	    !streamer.readFloat (savedPan) || !streamer.readFloat (savedMono))
		return Steinberg::kResultFalse;

	bypassActive = (savedBypass != 0.f);
	lastGainNormalized = savedGain;
	lastPanNormalized = savedPan;
	monoSumActive = (savedMono != 0.f);

	chain.setGainTarget (linearGainFromNormalized (lastGainNormalized));
	chain.setPanTarget (panFromNormalized (lastPanNormalized));
	chain.setMonoSumEnabled (monoSumActive);
	return Steinberg::kResultTrue;
}

//------------------------------------------------------------------------
Steinberg::tresult PLUGIN_API Processor::getState (Steinberg::IBStream* state)
{
	Steinberg::IBStreamer streamer (state, kLittleEndian);
	streamer.writeFloat (bypassActive ? 1.f : 0.f);
	streamer.writeFloat (static_cast<float> (lastGainNormalized));
	streamer.writeFloat (static_cast<float> (lastPanNormalized));
	streamer.writeFloat (static_cast<float> (lastMonoNormalized));
	return Steinberg::kResultTrue;
}

} // namespace Radixhomework
