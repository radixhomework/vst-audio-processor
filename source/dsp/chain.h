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
#include <span>

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
	/// per the plugin-core bus contract. The bus struct itself is only read;
	/// sample data is modified through its channel pointers.
	void process (const Steinberg::Vst::AudioBusBuffers& bus, Steinberg::int32 numSamples,
	              Steinberg::int32 sampleSize)
	{
		const auto count = static_cast<std::size_t> (numSamples);
		if (bus.numChannels >= 2)
		{
			if (sampleSize == Steinberg::Vst::kSample64)
				processStereo (std::span (bus.channelBuffers64[0], count),
				               std::span (bus.channelBuffers64[1], count));
			else
				processStereo (std::span (bus.channelBuffers32[0], count),
				               std::span (bus.channelBuffers32[1], count));
		}
		else if (sampleSize == Steinberg::Vst::kSample64)
			processMono (std::span (bus.channelBuffers64[0], count));
		else
			processMono (std::span (bus.channelBuffers32[0], count));
	}

private:
	template <typename Sample>
	void processStereo (std::span<Sample> left, std::span<Sample> right)
	{
		constexpr float halfPi = 1.57079632679489661923f;

		for (auto lIt = left.begin (), rIt = right.begin (); lIt != left.end (); ++lIt, ++rIt)
		{
			auto& l = *lIt;
			auto& r = *rIt;
			// 1) mono-sum: (L + R) * 0.5 on both channels. A full-scale
			// correlated input cannot clip (-6 dB).
			if (monoSumEnabled_)
			{
				const float mono = (static_cast<float> (l) + static_cast<float> (r)) * 0.5f;
				l = static_cast<Sample> (mono);
				r = static_cast<Sample> (mono);
			}

			// 2) gain, per-sample from the ~15 ms ramp.
			const float g = gain.nextValue ();
			l *= g;
			r *= g;

			// 3) pan/balance: favored channel stays at unity, opposite
			// follows cos(|p| * pi / 2) — constant-power taper, center at
			// 1.0 / 1.0.
			const float p = panPosition.nextValue ();
			const float attenuate = std::cos (std::fabs (p) * halfPi);
			if (p < 0.f)
				r *= attenuate;
			else
				l *= attenuate;
		}
	}

	template <typename Sample>
	void processMono (std::span<Sample> channel)
	{
		for (auto& sample : channel)
			sample *= gain.nextValue ();
	}

	SmoothedValue gain;
	SmoothedValue panPosition;
	bool monoSumEnabled_ {false};
};

} // namespace Radixhomework
