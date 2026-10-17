# Design: bootstrap-vst3-plugin

## Context

Greenfield: the repo holds only documentation scaffolding (AGENTS.md, PRODUCT.md
and ARCHITECTURE.md stubs, GPL-3 LICENSE, OpenSpec). There is no build system or
code yet. Round 1 targets Windows-only builds with Cubase as the manual host
test bench; later feature rounds (tuner, dynamics, EQ, saturation/distortion,
modulation) will grow around the same processing skeleton. See proposal.md for
motivation and the four delta specs for the behavioral contract.

## Goals / Non-Goals

**Goals:**

- One CMake configure/build produces an installable Windows `.vst3` bundle.
- A processing core organized as fixed-order stages so future modules slot in
  without restructuring (the "basement" quality the proposal promises).
- All host interaction through the VST3 SDK; all UI through VSTGUI; no other
  third-party DSP/UI libraries.
- Source code stays platform-portable (no Win32 outside SDK/VSTGUI platform
  layers) so macOS later is build configuration, not a rewrite.

**Non-Goals:**

- No macOS build/signing configuration this round.
- No installer or auto-update packaging; the deliverable is the `.vst3` bundle
  (zipped for GitHub releases later).
- No heavy test suite (per AGENTS.md testing stance): VST3 validator + manual
  Cubase checklist only.
- No preset system beyond the host's default program.

## Decisions

### D1 — Framework: raw VST3 SDK + VSTGUI

Build directly on Steinberg's VST3 SDK (which bundles VSTGUI 4). JUCE was
rejected: it would change the effective license terms (AGPL-3 vs GPL-3) and is a
heavier framework than a utility plugin needs. iPlug2 was viable but has a
smaller community and less reference material than the SDK samples for the
host-contract work this project must get exactly right (buses, bypass,
threading).

### D2 — SDK acquisition: CMake FetchContent, pinned release tag

`FetchContent` pulls the SDK at a pinned stable release tag, overridable via a
`VST3_SDK_DIR` cache variable for local development. Alternatives: git
submodule (extra setup step for contributors, no functional gain here) and a
system-installed SDK (non-reproducible across machines/CI). The exact tag is
chosen at implementation time (latest stable `v3.7.x`-family release) and
recorded in the top-level CMakeLists.

License note: the VST3 SDK is now MIT-licensed only — Steinberg retired the
former GPLv3/proprietary dual license — and VSTGUI is BSD-3. Both are
GPL-compatible, so publishing the plugin under GPL-3 is unobstructed; the MIT
and BSD notices ship in a third-party notices file (task 6.5). If an older
pre-MIT SDK tag were ever pinned instead, its GPLv3 option would apply — the
plugin stays GPL-3-compliant under either regime. The "VST" trademark rules
unchanged: keep "VST" out of the product name ("Audio Processor" complies).

### D3 — Source layout and language

C++20, single VST3 module:

```
source/
  factory.cpp          plug-in factory / module entry
  processor.cpp/.h     audio processor (bus setup, bypass, process loop)
  controller.cpp/.h    edit controller (parameters, editor lifecycle)
  params.h             parameter IDs + ranges (single source of truth)
  version.h            version, name, vendor, class IDs
  dsp/
    chain.h            fixed-order stage runner (mono-sum -> gain -> pan)
    smoothed_value.h   click-free parameter ramp helper (shared)
    meter_state.h      lock-free meter/clip publication (atomics)
  ui/
    editor.cpp/.h      panel construction and parameter bindings
    meter_view.h/.cpp  the one custom widget (RMS fill, peak line, hold)
```

Class IDs and UIDs are generated once and committed; they must never change
after the first release (host projects would lose assignments).

### D4 — DSP chain and parameter smoothing

- Parameters arrive per block (VST3 parameter queue). `SmoothedValue` ramps
  linearly toward the target over ~15 ms; gain and pan coefficients are
  recomputed per sample from the ramp. This satisfies the click-free
  automation requirement at negligible cost.
- Gain: dB parameter mapped to linear amplitude. Normalized mapping:
  index 0 = −∞ (mute), then −60..+12 dB in 0.1 dB steps (721 discrete steps).
- Pan (balance, stereo instances): favored channel pinned at 1.0, opposite
  channel = cos(|p| · π/2), p ∈ [−1, 1] — constant-power taper, center
  = 1.0/1.0. Mono instances skip the stage entirely.
- Mono-sum: `(L + R) × 0.5` on both channels (−6 dB; a full-scale correlated
  input cannot clip). No-op on mono instances.
- Chain order fixed: mono-sum → gain → pan. Future rounds insert stages, never
  reorder.
- Bypass: pass-through copy input→output; metering keeps measuring (the output
  *is* the input then). No crossfade on bypass toggling this round (host
  typically compensates; revisit if clicks are heard in Cubase).

### D5 — Metering data path

The processor computes, per output channel and per block: block peak (max |x|)
and RMS (root-mean-square over the block), then applies display ballistics
(fast attack, ~300 ms release) before publishing to `MeterState`:
`std::atomic<float>` levels + `std::atomic<bool>` clip flags, latched by the
audio thread, cleared by the UI. No locks, no allocation after `setActive(true)`.
The meter view polls on a ~30 ms VSTGUI timer; peak-hold (2 s hold, then fall)
is view-side visual state, so it resets when the editor reopens — acceptable.

### D6 — UI construction: programmatic VSTGUI

The editor builds its view tree in C++ (SDK-samples style) rather than a
`.uidesc` XML. Rationale: only one custom widget exists and precise control of
the meter view matters; XML indirection buys nothing at this size. Knobs and
buttons bind through VSTGUI parameter attachments so all moves are host
automation; meters are *not* exposed as VST3 output parameters (keeps the
host's generic editor and automation lanes clean). Panel is a fixed
~420×220 px; exact pixel layout is an implementation detail.

### D7 — Build, CI, and identity

- CMake ≥ 3.22, MSVC v143 (VS 2022), Release config produces the `.vst3`
  bundle under `VST3/` in the build dir.
- GitHub Actions (`windows-latest`): configure, build, run the SDK's validator
  against the built bundle, upload the bundle as an artifact. Sonar/CodeQL
  bindings per AGENTS.md quality gates are wired as far as the pipeline allows;
  if org-level tokens are not yet available, that wiring is recorded as a
  blocked follow-up rather than silently skipped.

## Risks / Trade-offs

- [FetchContent needs network on clean builds] → pinned tag is cached by CMake
  and CI; `VST3_SDK_DIR` override unblocks offline work.
- [Custom meter view is the riskiest UI piece] → isolated behind `meter_view`
  with a trivial fallback (peak-only bar); validated manually with sine signals
  at known levels.
- [SDK CMake macros shift between releases] → exact tag pinned; upgrades are
  deliberate, reviewed changes.
- [Host quirks (bus negotiation, bypass behavior) only surface in Cubase] →
  validator covers the contract; tasks include a manual Cubase checklist (mono
  track, stereo track, automation, bypass sync).
- [Zipper noise would pass the validator silently] → shared `SmoothedValue`
  helper is the single path for continuous parameters; manual click test in the
  checklist.
- [Meter polling on the UI thread] → atomic reads at 30 Hz; negligible cost.

## Migration Plan

Not applicable — greenfield first increment. Work lands on a `feat/*` branch
opened as a PR per AGENTS.md; rollback is closing the branch.

## Open Questions

None blocking. Implementation-time picks that do not affect specs or tasks:
exact SDK release tag, exact panel pixel dimensions and colors.
