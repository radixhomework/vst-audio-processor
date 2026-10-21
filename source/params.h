//------------------------------------------------------------------------
// Parameter IDs, ranges and laws — single source of truth (design D3),
// shared by the edit controller (registration, display) and the processor
// (queue handling, DSP).
//
// IDs are stable once released: hosts store them inside automation data.
//------------------------------------------------------------------------
#pragma once

#include "pluginterfaces/vst/vsttypes.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace Radixhomework {

//------------------------------------------------------------------------
// Parameter IDs
//------------------------------------------------------------------------
constexpr Steinberg::Vst::ParamID kBypassId = 1; ///< Standard bypass (kIsBypass).
constexpr Steinberg::Vst::ParamID kGainId = 2;
constexpr Steinberg::Vst::ParamID kPanId = 3;
constexpr Steinberg::Vst::ParamID kMonoId = 4;

//------------------------------------------------------------------------
// Gain law (audio-controls spec): stepCount 721 -> 722 discrete values.
// Index 0 = -inf (full mute); indices 1..721 cover -60.0 .. +12.0 dB in
// 0.1 dB steps. Default 0 dB.
//------------------------------------------------------------------------
constexpr int kGainStepCount = 721;
constexpr float kGainMinDb = -60.f;
constexpr float kGainMaxDb = 12.f;
constexpr float kGainStepDb = 0.1f;
constexpr float kGainDefaultDb = 0.f;

inline constexpr float kMinusInftyDb = -std::numeric_limits<float>::infinity ();

inline int gainNormalizedToIndex (Steinberg::Vst::ParamValue normalized)
{
	const int index = static_cast<int> (std::lround (normalized * kGainStepCount));
	return std::clamp (index, 0, kGainStepCount);
}

inline Steinberg::Vst::ParamValue gainIndexToNormalized (int index)
{
	return static_cast<Steinberg::Vst::ParamValue> (index) / kGainStepCount;
}

inline int dbToGainIndex (float db)
{
	db = std::clamp (db, kGainMinDb, kGainMaxDb);
	return static_cast<int> (std::lround ((db - kGainMinDb) / kGainStepDb)) + 1;
}

inline float gainIndexToDb (int index)
{
	if (index <= 0)
		return kMinusInftyDb;
	return std::min (kGainMinDb + (index - 1) * kGainStepDb, kGainMaxDb);
}

/// dB -> linear amplitude (-inf dB maps to 0: digital silence).
inline float linearGainFromDb (float db)
{
	return std::pow (10.f, db * 0.05f);
}

inline Steinberg::Vst::ParamValue gainDefaultNormalized ()
{
	return gainIndexToNormalized (dbToGainIndex (kGainDefaultDb));
}

/// Normalized parameter value -> linear gain, through the discrete law.
inline float linearGainFromNormalized (Steinberg::Vst::ParamValue normalized)
{
	return linearGainFromDb (gainIndexToDb (gainNormalizedToIndex (normalized)));
}

//------------------------------------------------------------------------
// Pan law (audio-controls spec): -100..+100 user units map to -1..+1
/// internally; default 0 (center). Display format C / L<n> / R<n>.
//------------------------------------------------------------------------
constexpr float kPanMin = -1.f;
constexpr float kPanMax = 1.f;
constexpr float kPanDefault = 0.f;

inline float panFromNormalized (Steinberg::Vst::ParamValue normalized)
{
	return static_cast<float> (normalized) * 2.f - 1.f;
}

inline Steinberg::Vst::ParamValue panToNormalized (float pan)
{
	return (static_cast<Steinberg::Vst::ParamValue> (pan) + 1.) * 0.5;
}

inline Steinberg::Vst::ParamValue panDefaultNormalized ()
{
	return panToNormalized (kPanDefault);
}

//------------------------------------------------------------------------
// Mono-sum switch: off by default (audio-controls spec).
//------------------------------------------------------------------------
constexpr Steinberg::Vst::ParamValue kMonoDefaultNormalized = 0.;

} // namespace Radixhomework
