# ARCHITECTURE.md — how this project is built

> Living document. Update it in the same change that alters structure,
> deployment, or technical choices — see AGENTS.md, "Product & architecture
> documentation".

## Overview

A single VST3 module (`AudioProcessor.vst3`) built with CMake + MSVC on
top of the Steinberg VST3 SDK (pinned via `FetchContent`) and VSTGUI 4.
The processor side owns the audio DSP and metering; the controller side
owns parameters and the editor; a VST3 data-exchange queue carries meter
snapshots from the audio thread to the UI lock-free.

```
                 host (Cubase)
      buses / process()   parameters / automation
   ┌────────┴───────────────────┬──────────────┐
   │ Processor                  │ Controller   │
   │  DspChain (mono→gain→pan)  │  params      │
   │  Metering ──MeterPacket──► │  meter cache │
   │  MeterState (latches)      │  Editor ─────┼──► MeterView (polls ~30 ms)
   │        ▲    clip reset msg │              │      knobs/buttons/readouts
   └────────┼───────────────────┴──────────────┘
            └──────────────────────────────────┘
   (data exchange over the processor↔controller connection point;
    clip reset travels back as an IMessage "Meter/ResetClip")
```

## Components

| Component | Technology | Responsibility | Location |
|---|---|---|---|
| Factory/module entry | VST3 SDK | Registers processor + controller classes; identity from `version.h` | `source/factory.cpp` |
| Processor | VST3 SDK (`AudioEffect`) | Bus contract, bypass, process loop, state persistence, meter publishing | `source/processor.h/.cpp` |
| Controller | VST3 SDK (`EditController`, `IDataExchangeReceiver`) | Parameter registration/display, editor creation, meter cache, clip reset | `source/controller.h/.cpp` |
| Params | — | Parameter IDs, gain/pan laws, defaults (single source of truth) | `source/params.h` |
| DSP chain | C++20 header-only | Fixed order mono-sum → gain → pan; no-op rules on mono buses | `source/dsp/chain.h` |
| Smoothing | C++20 header-only | Linear ~15 ms ramp shared by continuous parameters | `source/dsp/smoothed_value.h` |
| Metering | C++20 + atomics | Per-block peak/RMS, ballistics, clip latch, `MeterPacket` type | `source/dsp/meter_state.h` |
| Editor | VSTGUI 4 (programmatic) | Resizable panel (base 420×220, 21:11 aspect, 315×165–840×440): vector knobs, readouts, mono/bypass buttons, meters | `source/ui/editor.h/.cpp` |
| Meter view | VSTGUI 4 custom widget | RMS fill, peak line, 2 s falling hold, dB scale, clickable clip LED | `source/ui/meter_view.h/.cpp` |

## Data flow

**Audio:** host buffers → copy to outputs → `DspChain` (mono-sum → gain →
pan, in place) → `Metering.measure` (post-pan; also on the bypass path,
where output = input) → `MeterPacket` sent via
`Vst::DataExchangeHandler` (lock-free; no-op unless the queue is open).
While bypassed the chain is skipped entirely (strict pass-through).

**Parameters:** host automation → `inputParameterChanges` queues, read per
block (including zero-sample flush calls) → chain targets / bypass /
mono. UI moves → `performEdit` (host-mediated) → same queues.

**Metering → UI:** processor publishes `MeterPacket{rms, peak, clipFlags}`
per block; the controller (`IDataExchangeReceiver`) copies it into a
`MeterState` cache of atomics; `MeterView` polls on a ~30 ms VSTGUI timer.
Peak *hold* is view-side state (resets when the editor reopens — accepted).
Clip reset: LED click → controller clears its cache and sends the
`Meter/ResetClip` message → processor clears the latched flags.

**Persistence:** `Processor::getState` writes `[bypass, gain, pan, mono]`
as normalized floats; `Controller::setComponentState` mirrors them into
the parameters (layout is shared — change both together).

## Data & storage

- No files, no database. The only persistent state is the host-managed
  plugin state stream described above.
- Class IDs and parameter IDs are immutable once released (hosts store
  them in projects).

## Key technical decisions

| Decision | Alternative considered | Rationale |
|---|---|---|
| D1: raw VST3 SDK + VSTGUI | JUCE (AGPL conflict), iPlug2 (smaller community) | License compatibility with GPL-3; host-contract control |
| D2: FetchContent, pinned `v3.7.9_build_61` | Git submodule, system SDK | Reproducible; `VST3_SDK_DIR` override for offline |
| D3: C++20, `source/` layout, `dsp/` + `ui/` | — | Platform-portable core, UI isolated |
| D4: fixed chain order + shared `SmoothedValue` | Per-parameter ad-hoc ramps | Click-free automation via one code path; stages insert, never reorder |
| D5: block peak/RMS + ballistics, atomic publication, data exchange to UI | Output parameters for meters | Keeps host automation lanes clean; lock-free audio→UI |
| D6: programmatic VSTGUI panel | `.uidesc` XML | One custom widget; precise meter control |
| D7: CI on pinned `windows-2022` runner | `windows-latest` (now VS 2026-only) | Toolchain pinned to local VS 2022/v143; upgrades are deliberate |

## Build & run

Prerequisites: Visual Studio 2022 (MSVC v143), CMake ≥ 3.22, network for
the first configure (SDK download) or `-DVST3_SDK_DIR=<path>` to an
existing SDK checkout.

```bash
# configure + build (from the repository root)
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -T v143
cmake --build build --config Release

# run the VST3 validator against the bundle
./build/bin/Release/validator.exe ./build/VST3/Release/AudioProcessor.vst3
```

The installable bundle lands at `build/VST3/Release/AudioProcessor.vst3/`.
For manual host testing, copy (or link) it into the host's VST3 folder.

## Deployment

- The deliverable is the `.vst3` bundle; no installer yet.
- CI uploads the bundle as a workflow artifact on tag pushes
  (`AudioProcessor-<tag>-windows-vst3`); releases zip that artifact.

## Quality & CI

- **Build workflow** (`.github/workflows/build.yml`): `windows-2022`
  runner; configure → Release build (under Sonar's build wrapper) → VST3
  validator → SonarCloud analysis (gated on the `SONAR_TOKEN` secret;
  analysis limited to `source/`, coverage excluded per the AGENTS.md
  testing stance) → bundle artifact on tags. Runs on `main`, PRs, tags.
- **CodeQL workflow** (`.github/workflows/codeql.yml`): C/C++ analysis on
  `main` and PRs.
- Definition of "passing" (AGENTS.md): green CI, Sonar quality gate OK
  with no new issues on changed code, no CodeQL alerts, PR comments
  addressed.

## Environments

| Environment | Purpose | Notes |
|---|---|---|
| Dev machine (Windows + VS 2022) | Daily build, validator, scratch DSP verification | Harnesses live outside the repo (testing stance) |
| GitHub Actions (`windows-2022`) | CI: build + validator + Sonar + CodeQL | Pinned runner image |
| Cubase (manual) | Host-contract + audible checks per release | Checklist recorded in the PR |
