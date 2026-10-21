//------------------------------------------------------------------------
// Linear parameter smoothing shared by all continuous coefficients
// (design D4): the single code path that keeps automation click-free.
// Ramps linearly toward the target over ~15 ms; a new target arriving
// mid-ramp simply recomputes the increment from the current position.
//------------------------------------------------------------------------
#pragma once

namespace Radixhomework {

class SmoothedValue
{
public:
	void setup (double sampleRate)
	{
		// ~15 ms ramp length, at least one sample.
		rampSamples = static_cast<float> (0.015 * sampleRate);
		if (rampSamples < 1.f)
			rampSamples = 1.f;
	}

	/// Starts settled at value (used on activation).
	void setCurrent (float value)
	{
		current = target = value;
		increment = 0.f;
	}

	void setTarget (float value)
	{
		target = value;
		increment = (target - current) / rampSamples;
	}

	float nextValue ()
	{
		current += increment;
		// Clamp at the target once the ramp overshoots (also the exit for a
		// finished ramp: increment becomes a no-op only via this clamp).
		if ((increment > 0.f && current > target) || (increment < 0.f && current < target))
			current = target;
		return current;
	}

private:
	float rampSamples {1.f};
	float current {0.f};
	float target {0.f};
	float increment {0.f};
};

} // namespace Radixhomework
