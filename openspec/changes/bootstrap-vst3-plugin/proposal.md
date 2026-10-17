# Proposal: bootstrap-vst3-plugin

## Why

The repository has documentation and license scaffolding but no product code. The
goal is a GPL-3-licensed VST3 audio *effect* plugin — "Audio Processor" by
Radixhomework — for use in Cubase. This change bootstraps the plugin skeleton and
delivers Round 1, the "basement" utility feature set (gain/pan, mono/stereo
handling, metering), on a real-time-safe processing core that later feature
rounds (tuner, dynamics, EQ, saturation/distortion, modulation) will extend.

## What Changes

- New VST3 plugin project built with CMake + Visual Studio (Windows-only for
  this round) using the Steinberg VST3 SDK and VSTGUI; source code stays
  platform-portable so a macOS build later is a build-configuration job.
- Plugin identity: name "Audio Processor", vendor "Radixhomework".
- Bus configurations: 1-in/1-out (mono) and 2-in/2-out (stereo).
- Audio chain: **mono-sum switch → gain → pan**.
  - Gain −∞..+12 dB, 0 dB default, zipper-free parameter smoothing.
  - Pan −100..+100, constant-power law (balance control on stereo instances;
    inert on mono instances — a 1-in/1-out arrangement has nothing to pan
    into).
  - Mono switch sums L+R (stereo instances; no-op on mono instances).
- VST3 standard bypass parameter supported.
- Post-pan metering: peak + RMS with falling peak hold (~2 s); clip detection at
  ~0 dBFS with latching, click-to-reset indicators.
- Minimal VSTGUI editor: gain/pan knobs with numeric readouts, mono toggle,
  bypass button, L/R meter bars (RMS fill + peak line + hold + dB scale), clip
  LEDs.
- PRODUCT.md and ARCHITECTURE.md stubs filled in (first task touching the repo).
- Light CI: GitHub Actions workflow doing a Windows build + VST3 validator run.

Deliberately out of scope for this change: preset browser UI, resizable or
skinnable UI, separate input/output gains, M/S width controls, LUFS loudness
metering, and all later-round features (tuner, dynamics, EQ, saturation,
distortion, modulation).

## Capabilities

### New Capabilities

- `plugin-core`: plugin identity, mono/stereo bus configurations, VST3 bypass
  handling, and the real-time processing contract (no allocations or locks on
  the audio thread).
- `audio-controls`: the audible processing behavior of the chain — gain, pan,
  and mono-sum, including parameter ranges, laws, and smoothing.
- `metering`: post-pan peak/RMS detection, peak hold, and clip detection.
- `plugin-ui`: the editor panel — knobs, numeric readouts, mono/bypass buttons,
  meter bars with dB scale, and clip LED reset.

### Modified Capabilities

(none — the spec inventory is empty; this is the project's first change)

## Impact

- **Code**: introduces the entire source tree, CMake build scripts, and
  third-party dependencies: Steinberg VST3 SDK (MIT — Steinberg retired the
  former GPLv3/proprietary dual license) and VSTGUI 4 (BSD-3). Both are
  GPL-compatible; their license notices ship in a third-party notices file.
- **Docs**: fills the currently-stub PRODUCT.md and ARCHITECTURE.md.
- **CI**: introduces the first GitHub Actions workflow (Windows build + VST3
  validator); the AGENTS.md quality gates (Sonar, CodeQL) will run against this
  pipeline.
- **Tooling**: requires CMake and Visual Studio 2022 (MSVC v143) locally;
  Cubase serves as the manual host test bench.
