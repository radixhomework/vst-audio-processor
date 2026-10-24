# PRODUCT.md — what this project is

> Living document. Every agent and contributor reads this before working on
> the code. Update it in the same change that alters the product — see
> AGENTS.md, "Product & architecture documentation".

## What it is

**Audio Processor** is a VST3 audio effect plug-in (currently Windows,
Cubase as the reference host) providing a small, dependable set of utility
controls: gain, pan/balance, a mono fold-down switch, post-processing
level metering with peak hold and clip indication, and standard bypass.
It is developed by Radixhomework in incremental rounds; Round 1 (the
current state) delivers the "basement": the utility feature set above on
a real-time-safe processing core that later rounds extend.

## Who it's for

Home- and project-studio users who insert a utility gain/pan/monitoring
stage on mono or stereo tracks in Cubase, and who prefer a small, focused,
free (GPL-3) tool. It is also the Radixhomework learning vehicle for
VST3/C++ audio development, which shapes its "boring and correct" scope.

## Scope

### In scope (Round 1 — implemented)

- Gain: −∞ (mute) .. +12 dB in 0.1 dB steps, 0 dB default, click-free
  automation (~15 ms smoothing).
- Pan/balance: −100..+100, constant-power taper, center default; inert on
  mono instances.
- Mono-sum switch: (L+R)×0.5, off by default; no-op on mono instances.
- Metering: per-channel peak + RMS with ~2 s falling peak hold and
  latching clip indicators (click to reset), shown against a dB scale.
- Standard VST3 bypass with strict pass-through.
- Mono (1-in/1-out) and stereo (2-in/2-out) arrangements; 32- and 64-bit
  float processing; 44.1–192 kHz; any buffer size.

### Out of scope (deliberately)

- Preset browser UI, skinnable UI, separate input/output gains,
  M/S width controls, LUFS loudness metering.
- macOS builds (source stays platform-portable; a macOS build is build
  configuration, not a rewrite).
- A unit-test suite: verification is the VST3 SDK validator plus a manual
  Cubase checklist (see AGENTS.md testing stance).

### Roadmap (future rounds, not yet specified)

| Round | Theme | Candidate features |
|---|---|---|
| R2 | Tuning | Instrument tuner |
| R3 | Dynamics | Compressor/gate |
| R4 | Tone | EQ, saturation/distortion |
| R5 | Motion | Modulation effects |

## Key features

| Feature | What it does | Status |
|---|---|---|
| Gain | −∞..+12 dB, 0.1 dB steps, zipper-free | Done (Round 1) |
| Pan/balance | Constant-power, favored channel at unity | Done (Round 1) |
| Mono fold-down | (L+R)×0.5 to both channels | Done (Round 1) |
| Metering | Peak + RMS, hold, clip latch/reset | Done (Round 1) |
| Bypass | Standard VST3 bypass, strict pass-through | Done (Round 1) |
| Editor | Programmatic VSTGUI panel, host-automated controls | Done (Round 1) |
| Resizable UI | Corner-drag resize, proportional scaling, min/max bounds | In progress (Round 1 scope update) |

## Constraints & non-functional requirements

- License: GPL-3 (project code). The pinned VST3 SDK v3.7.9 is consumed
  under its GPLv3 option; VSTGUI is BSD — see `THIRD-PARTY-NOTICES.md`.
- Host/format: VST3 only, Cubase as reference host, Windows-first.
- Real-time safety: no allocation, locking or blocking on the audio path
  once activated.
- "VST" must not appear in the product name (Steinberg trademark rules);
  plug-in identity and class IDs are immutable once released.
- Verification: VST3 validator (CI, every PR) + manual Cubase checklist
  per release; SonarCloud quality gate on changed code.

## Decisions log

Key decisions D1–D7 (framework choice, SDK acquisition, source layout,
DSP chain and smoothing, metering data path, UI construction, CI) are
recorded and maintained in `ARCHITECTURE.md`.
