//------------------------------------------------------------------------
// Metering (design D5): the audio thread measures per-channel block peak
// and RMS of the final output (post-chain, bypass included), applies
// display ballistics (fast attack, ~300 ms release) and publishes
// through MeterState's atomics. Clip flags latch on the audio thread and
// are cleared by the UI (task 5.4). No locks, no allocation on the audio
// path.
//------------------------------------------------------------------------
#pragma once

#include "pluginterfaces/vst/ivstaudioprocessor.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <span>

namespace Radixhomework {

constexpr int kMaxMeterChannels = 2;

//------------------------------------------------------------------------
// Lock-free shared state: audio thread writes levels / latches clip,
// UI thread reads levels / clears clip. Plain atomic loads/stores are
// enough — the values are display-only and the peak-hold logic lives
// view-side.
class MeterState
{
public:
	void setRms (int channel, float value) { rms[channel].store (value); }

	float getRms (int channel) const { return rms[channel].load (); }

	void setPeak (int channel, float value) { peak[channel].store (value); }

	float getPeak (int channel) const { return peak[channel].load (); }

	void latchClip (int channel) { clip[channel].store (true); }

	/// Restores a latched state received from the processor (controller side).
	void setClip (int channel, bool latched) { clip[channel].store (latched); }

	bool getClip (int channel) const { return clip[channel].load (); }

	/// Clear path the UI calls when the user clicks a lit clip indicator.
	void clearClip (int channel) { clip[channel].store (false); }

private:
	std::array<std::atomic<float>, kMaxMeterChannels> rms {};
	std::array<std::atomic<float>, kMaxMeterChannels> peak {};
	std::array<std::atomic<bool>, kMaxMeterChannels> clip {};
};

//------------------------------------------------------------------------
// Snapshot sent per block from the processor to the controller via VST3
// data exchange (design D5/D6): the lock-free audio-to-UI channel.
struct MeterPacket
{
	float rms[kMaxMeterChannels];
	float peak[kMaxMeterChannels];
	Steinberg::uint32 clipFlags; ///< one bit per channel
};

/// Data-exchange queue identity for the meter stream.
constexpr Steinberg::uint32 kMeterQueueContextId = 0x4D455445; // 'METE'

//------------------------------------------------------------------------
// Audio-side measurement: per-channel block peak/RMS with instant attack
// and ~300 ms exponential release, publishing into a MeterState.
class Metering
{
public:
	/// Called on activation: fixes the release time constant from the
	/// sample rate and resets the ballistics state.
	void prepare (double sampleRate)
	{
		releaseSamples = kReleaseTimeSeconds * static_cast<float> (sampleRate);
		for (int ch = 0; ch < kMaxMeterChannels; ++ch)
		{
			smoothedRms[ch] = 0.f;
			smoothedPeak[ch] = 0.f;
		}
	}

	void measure (const Steinberg::Vst::AudioBusBuffers& bus, Steinberg::int32 numSamples,
	              Steinberg::int32 sampleSize, MeterState& state)
	{
		const int channels = std::min<int> (bus.numChannels, kMaxMeterChannels);
		if (channels <= 0 || numSamples <= 0)
			return;

		const auto count = static_cast<std::size_t> (numSamples);
		const float releaseFactor =
		    std::exp (-static_cast<float> (numSamples) / std::max (releaseSamples, 1.f));

		for (int ch = 0; ch < channels; ++ch)
		{
			if (sampleSize == Steinberg::Vst::kSample64)
				measureChannel (std::span (bus.channelBuffers64[ch], count), ch, releaseFactor,
				                state);
			else
				measureChannel (std::span (bus.channelBuffers32[ch], count), ch, releaseFactor,
				                state);
		}
	}

private:
	template <typename Sample>
	void measureChannel (std::span<Sample> samples, int channel, float releaseFactor,
	                     MeterState& state)
	{
		float sumSquares = 0.f;
		float blockPeak = 0.f;
		for (const Sample sample : samples)
		{
			const float magnitude = std::fabs (static_cast<float> (sample));
			sumSquares += magnitude * magnitude;
			blockPeak = std::max (blockPeak, magnitude);
		}

		// Clip (metering spec): latches when any sample reaches full scale
		// (|x| >= 1.0), equivalent to the block peak reaching 1.0.
		if (blockPeak >= 1.f)
			state.latchClip (channel);

		const float blockRms = std::sqrt (sumSquares / static_cast<float> (samples.size ()));

		// Ballistics: instant attack, exponential release.
		smoothedRms[channel] = std::max (blockRms, smoothedRms[channel] * releaseFactor);
		smoothedPeak[channel] = std::max (blockPeak, smoothedPeak[channel] * releaseFactor);

		state.setRms (channel, smoothedRms[channel]);
		state.setPeak (channel, smoothedPeak[channel]);
	}

	static constexpr float kReleaseTimeSeconds = 0.3f;

	float releaseSamples {1.f};
	std::array<float, kMaxMeterChannels> smoothedRms {};
	std::array<float, kMaxMeterChannels> smoothedPeak {};
};

} // namespace Radixhomework
