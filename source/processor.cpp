#include "processor.h"

#include "version.h"

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

	// Placeholder bus layout; the exact mono/stereo contract lands in task 2.1.
	addAudioInput (STR16 ("Audio Input"), Steinberg::Vst::SpeakerArr::kStereo);
	addAudioOutput (STR16 ("Audio Output"), Steinberg::Vst::SpeakerArr::kStereo);
	return Steinberg::kResultTrue;
}

//------------------------------------------------------------------------
namespace {

// Placeholder processing (task 1.2): strict pass-through, or silence when no
// input bus is connected. Replaced by the DSP chain (mono-sum -> gain -> pan)
// in task 3.6.

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

} // namespace

//------------------------------------------------------------------------
Steinberg::tresult PLUGIN_API Processor::process (Steinberg::Vst::ProcessData& data)
{
	if (data.numSamples <= 0 || data.numOutputs <= 0)
		return Steinberg::kResultTrue;

	if (data.symbolicSampleSize != Steinberg::Vst::kSample32 &&
	    data.symbolicSampleSize != Steinberg::Vst::kSample64)
		return Steinberg::kResultFalse;

	if (data.numInputs <= 0)
		clearBus (data.outputs[0], data.numSamples, data.symbolicSampleSize);
	else
		copyBus (data.inputs[0], data.outputs[0], data.numSamples, data.symbolicSampleSize);

	return Steinberg::kResultTrue;
}

//------------------------------------------------------------------------
Steinberg::tresult PLUGIN_API Processor::setState (Steinberg::IBStream* /*state*/)
{
	// No persistent state yet; parameters arrive via the controller.
	return Steinberg::kResultTrue;
}

//------------------------------------------------------------------------
Steinberg::tresult PLUGIN_API Processor::getState (Steinberg::IBStream* /*state*/)
{
	return Steinberg::kResultTrue;
}

} // namespace Radixhomework
