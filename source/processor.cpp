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
// All processing state must exist before the first process call, so the
// audio path never allocates or blocks (plugin-core spec, real-time
// safety). There is nothing dynamic to allocate yet: the placeholder path
// works purely on the host-provided buffers; the chain state of tasks 3.x
// will be created on activation.
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

//------------------------------------------------------------------------
// The processing chain slot: mono-sum -> gain -> pan (tasks 3.x). Until
// the chain lands (task 3.6) this is the placeholder pass-through.
void processChain (Steinberg::Vst::ProcessData& data)
{
	passThrough (data);
}

} // namespace

//------------------------------------------------------------------------
// Host parameter changes: track the standard bypass parameter. The last
// point of the block wins; the placeholder chain has nothing to gate
// sample-accurately, and the real chain (task 3.6) will smooth at the
// transition like any parameter.
void Processor::updateBypassState (Steinberg::Vst::ProcessData& data)
{
	Steinberg::Vst::IParameterChanges* paramChanges = data.inputParameterChanges;
	if (!paramChanges)
		return;

	for (Steinberg::int32 i = 0; i < paramChanges->getParameterCount (); ++i)
	{
		Steinberg::Vst::IParamValueQueue* queue = paramChanges->getParameterData (i);
		if (!queue || queue->getParameterId () != kBypassId)
			continue;

		Steinberg::Vst::ParamValue value = 0;
		Steinberg::int32 sampleOffset = 0;
		const Steinberg::int32 numPoints = queue->getPointCount ();
		if (numPoints > 0 &&
		    queue->getPoint (numPoints - 1, sampleOffset, value) == Steinberg::kResultTrue)
			bypassActive = (value != 0.);
	}
}

//------------------------------------------------------------------------
Steinberg::tresult PLUGIN_API Processor::process (Steinberg::Vst::ProcessData& data)
{
	// Parameter queues are handled even for zero-sample flush calls
	// (numSamples == 0, no buffers): hosts use those to advance parameter
	// state, and saved state must reflect it.
	updateBypassState (data);

	if (data.numSamples <= 0 || data.numOutputs <= 0)
		return Steinberg::kResultTrue;

	if (data.symbolicSampleSize != Steinberg::Vst::kSample32 &&
	    data.symbolicSampleSize != Steinberg::Vst::kSample64)
		return Steinberg::kResultFalse;

	if (bypassActive)
	{
		// Bypass (plugin-core spec): input passed to the output unmodified;
		// gain/pan/mono cannot affect the audio while engaged.
		passThrough (data);
	}
	else
	{
		processChain (data);
	}
	return Steinberg::kResultTrue;
}

//------------------------------------------------------------------------
// Persistent state: the bypass flag must survive host project save/load
// (the validator checks controller sync through this round-trip; task 3.x
// extends the stream with the audio parameters).
Steinberg::tresult PLUGIN_API Processor::setState (Steinberg::IBStream* state)
{
	Steinberg::IBStreamer streamer (state, kLittleEndian);
	float savedBypass = 0.f;
	if (!streamer.readFloat (savedBypass))
		return Steinberg::kResultFalse;
	bypassActive = (savedBypass != 0.f);
	return Steinberg::kResultTrue;
}

//------------------------------------------------------------------------
Steinberg::tresult PLUGIN_API Processor::getState (Steinberg::IBStream* state)
{
	Steinberg::IBStreamer streamer (state, kLittleEndian);
	streamer.writeFloat (bypassActive ? 1.f : 0.f);
	return Steinberg::kResultTrue;
}

} // namespace Radixhomework
