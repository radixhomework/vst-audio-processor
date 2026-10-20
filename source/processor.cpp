#include "processor.h"

#include "version.h"

#include <cstring>

namespace Radixhomework {

//------------------------------------------------------------------------
Processor::Processor ()
{
	// Tells the host which controller class belongs to this processor.
	setControllerClass (audioProcessorControllerUID);
}

//------------------------------------------------------------------------
Steinberg::FUnknown* Processor::createInstance (void*)
{
	return (Steinberg::Vst::IAudioProcessor*)new Processor ();
}

//------------------------------------------------------------------------
Steinberg::tresult PLUGIN_API Processor::initialize (Steinberg::FUnknown* context)
{
	Steinberg::tresult result = AudioEffect::initialize (context);
	if (result != Steinberg::kResultTrue)
		return result;

	// Placeholder bus layout; the exact mono/stereo contract lands in task 2.1.
	addAudioInput (STR16 ("Audio Input"), Steinberg::Vst::SpeakerArr::kStereo);
	addAudioOutput (STR16 ("Audio Output"), Steinberg::Vst::SpeakerArr::kStereo);
	return Steinberg::kResultTrue;
}

//------------------------------------------------------------------------
Steinberg::tresult PLUGIN_API Processor::process (Steinberg::Vst::ProcessData& data)
{
	if (data.numSamples <= 0 || data.numOutputs <= 0)
		return Steinberg::kResultTrue;

	if (data.symbolicSampleSize != Steinberg::Vst::kSample32 &&
	    data.symbolicSampleSize != Steinberg::Vst::kSample64)
		return Steinberg::kResultFalse;

	auto& out = data.outputs[0];

	// Placeholder: strict pass-through (no input connected -> silence). The
	// processing chain (mono-sum -> gain -> pan) lands with task 3.6.
	if (data.numInputs <= 0)
	{
		for (Steinberg::int32 ch = 0; ch < out.numChannels; ++ch)
		{
			if (data.symbolicSampleSize == Steinberg::Vst::kSample32 && out.channelBuffers32)
				memset (out.channelBuffers32[ch], 0, data.numSamples * sizeof (Steinberg::Vst::Sample32));
			else if (out.channelBuffers64)
				memset (out.channelBuffers64[ch], 0, data.numSamples * sizeof (Steinberg::Vst::Sample64));
		}
		out.silenceFlags = (out.numChannels >= 64) ? ~Steinberg::uint64 (0)
		                                           : ((Steinberg::uint64 (1) << out.numChannels) - 1);
		return Steinberg::kResultTrue;
	}

	auto& in = data.inputs[0];
	for (Steinberg::int32 ch = 0; ch < out.numChannels; ++ch)
	{
		if (ch >= in.numChannels)
			continue;
		if (data.symbolicSampleSize == Steinberg::Vst::kSample32)
		{
			if (in.channelBuffers32[ch] != out.channelBuffers32[ch])
				memcpy (out.channelBuffers32[ch], in.channelBuffers32[ch],
				        data.numSamples * sizeof (Steinberg::Vst::Sample32));
		}
		else
		{
			if (in.channelBuffers64[ch] != out.channelBuffers64[ch])
				memcpy (out.channelBuffers64[ch], in.channelBuffers64[ch],
				        data.numSamples * sizeof (Steinberg::Vst::Sample64));
		}
	}
	out.silenceFlags = 0;
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
