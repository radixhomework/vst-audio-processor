//------------------------------------------------------------------------
// Fixed-order processing chain (design D4): mono-sum -> gain -> pan.
// The order never changes; future rounds insert stages, never reorder.
//
// On mono arrangements the mono-sum and pan stages are no-ops by spec
// (a 1-in/1-out arrangement has no image to fold or position), so only
// the gain stage runs.
//------------------------------------------------------------------------
#pragma once

#include "smoothed_value.h"

#include "pluginterfaces/vst/ivstaudioprocessor.h"

#include <cmath>

namespace Radixhomework {

class DspChain
{
public:
	/// Called on activation: fixes the ~15 ms ramp length from the sample
	/// rate and settles the smoothers at the current targets so processing
	/// starts without an unintended ramp.
	void prepare (double sampleRate, float gainLinear, float pan, bool monoSumEnabled)
	{
		gain.setup (sampleRate);
		panPosition.setup (sampleRate);
		gain.setCurrent (gainLinear);
		panPosition.setCurrent (pan);
		monoSumEnabled_ = monoSumEnabled;
	}

	void setGainTarget (float gainLinear) { gain.setTarget (gainLinear); }
	void setPanTarget (float pan) { panPosition.setTarget (pan); }
	void setMonoSumEnabled (bool enabled) { monoSumEnabled_ = enabled; }

	/// Processes one bus in place; numChannels is 1 (mono) or 2 (stereo)
	/// per the plugin-core bus contract.
	void process (Steinberg::Vst::AudioBusBuffers& bus, Steinberg::int32 numSamples,
	              Steinberg::int32 sampleSize)
	{
		if (bus.numChannels >= 2)
		{
			if (sampleSize == Steinberg::Vst::kSample64)
				processStereo (bus.channelBuffers64[0], bus.channelBuffers64[1], numSamples);
			else
				processStereo (bus.channelBuffers32[0], bus.channelBuffers32[1], numSamples);
		}
		else if (sampleSize == Steinberg::Vst::kSample64)
			processMono (bus.channelBuffers64[0], numSamples);
		else
			processMono (bus.channelBuffers32[0], numSamples);
	}

private:
	template <typename Sample>
	void processStereo (Sample* left, Sample* right, Steinberg::int32 numSamples)
	{
		constexpr float halfPi = 1.57079632679489661923f;

		for (Steinberg::int32 i = 0; i < numSamples; ++i)
		{
			// 1) mono-sum: (L + R) * 0.5 on both channels. A full-scale
			// correlated input cannot clip (-6 dB).
			if (monoSumEnabled_)
			{
				const float mono =
				    (static_cast<float> (left[i]) + static_cast<float> (right[i])) * 0.5f;
				left[i] = static_cast<Sample> (mono);
				right[i] = static_cast<Sample> (mono);
			}

			// 2) gain, per-sample from the ~15 ms ramp.
			const float g = gain.nextValue ();
			left[i] *= g;
			right[i] *= g;

			// 3) pan/balance: favored channel stays at unity, opposite
			// follows cos(|p| * pi / 2) — constant-power taper, center at
			// 1.0 / 1.0.
			const float p = panPosition.nextValue ();
			const float attenuate = std::cos (std::fabs (p) * halfPi);
			if (p < 0.f)
				right[i] *= attenuate;
			else
				left[i] *= attenuate;
		}
	}

	template <typename Sample>
	void processMono (Sample* channel, Steinberg::int32 numSamples)
	{
		for (Steinberg::int32 i = 0; i < numSamples; ++i)
			channel[i] *= gain.nextValue ();
	}

	SmoothedValue gain;
	SmoothedValue panPosition;
	bool monoSumEnabled_ {false};
};

} // namespace Radixhomework
