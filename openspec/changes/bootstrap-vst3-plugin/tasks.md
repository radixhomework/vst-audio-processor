# Tasks: bootstrap-vst3-plugin

## 1. Build system skeleton

- [x] 1.1 Top-level CMakeLists: C++20, FetchContent the VST3 SDK at a pinned
      stable tag (with `VST3_SDK_DIR` cache override), MSVC v143 settings, and a
      plugin target producing a `.vst3` bundle. Verify: configure + build
      succeeds and emits the bundle under `VST3/`.
- [x] 1.2 Plugin identity: `source/version.h` (name "Audio Processor", vendor
      "Radixhomework", version 0.1.0, committed class IDs that will never
      change) plus factory/module entry registering a placeholder processor and
      controller. Verify: the VST3 validator loads the bundle and reports the
      identity; the bundle appears in Cubase's plugin browser.
- [x] 1.3 GitHub Actions workflow (`windows-latest`): configure, build Release,
      run the SDK validator against the bundle, upload the bundle as artifact.
      Verify: workflow runs green on push.

## 2. Host contract (plugin-core spec)

- [x] 2.1 Bus arrangements: declare exactly mono 1-in/1-out and stereo 2-in/2-out
      in processor/controller bus init and arrangement checks. Verify: validator
      passes bus checks; Cubase offers the plugin on mono and stereo tracks and
      no sidechain/surround variants.
- [x] 2.2 Process setup: accept 32-bit and 64-bit float streams, 44.1–192 kHz,
      any buffer size; allocate all processing state in `setActive(true)` so the
      process loop is allocation-free. Verify: validator run across formats;
      code inspection confirms no allocation/lock in the process path.
- [x] 2.3 Bypass: implement the standard VST3 bypass parameter with strict
      input→output pass-through while engaged. Verify: validator bypass check
      passes; in Cubase, engaging bypass yields sample-identical pass-through
      (A/B on a test signal).

## 3. Parameters and DSP chain (audio-controls spec)

- [x] 3.1 Parameter set (`source/params.h` + controller): Gain with 721 steps
      (index 0 = −∞, then −60..+12 dB in 0.1 dB steps, default 0 dB), Pan
      −100..+100 (default 0), Mono toggle (default off); mark all automatable.
      Verify: Cubase lists parameters with correct ranges/defaults; values
      editable from the generic editor.
- [x] 3.2 `SmoothedValue` helper: linear ramp to target over ~15 ms, reused by
      gain and pan coefficients. Verify: exercised through the validator process
      run; single code path for continuous parameters.
- [x] 3.3 Gain stage: dB→linear conversion including the −∞ mute endpoint, with
      per-sample smoothing. Verify: minimum = digital silence; +12 dB on a
      −12 dBFS sine measures ≈ 0 dBFS at output.
- [x] 3.4 Pan/balance stage: favored channel at 1.0, opposite channel
      cos(|p|·π/2); stage skipped entirely on mono arrangements. Verify: hard
      left leaves L at unity and R silent; mono-instance audio unaffected by
      pan moves.
- [x] 3.5 Mono-sum stage: `(L+R) × 0.5` to both channels, off by default, no-op
      on mono arrangements. Verify: full-scale correlated input (L=R) produces
      no clip with mono engaged; distinct L/R fold to identical outputs.
- [x] 3.6 Chain wiring: fixed order mono-sum → gain → pan, parameter queue
      handled per block in process. Verify: combined scenario — mono engaged +
      gain −6 dB on full-scale correlated input yields −6 dBFS on both outputs.

## 4. Metering (metering spec)

- [x] 4.1 `MeterState` (atomics for L/R levels + clip flags) and per-block
      peak/RMS measurement with display ballistics, tapped post-pan (including
      during bypass). Verify: levels respond to gain changes during a validator
      run / debug host; no locks or allocations introduced in the process path.
- [x] 4.2 Clip detection: latch when |sample| ≥ 1.0 per output channel; provide
      the atomic clear path the UI will call. Verify: latch engages on a
      full-scale tone and stays latched until cleared (cleared via UI in 5.4).

## 5. Editor UI (plugin-ui spec)

- [x] 5.1 Editor skeleton: controller `createEditor` building a fixed
      ~420×220 px panel programmatically in VSTGUI; gain/pan knobs and buttons
      bound via parameter attachments. Verify: editor opens in Cubase; knob
      moves are recorded by host automation.
- [x] 5.2 Knob readouts: gain text in dB showing "−∞ dB" at minimum; pan text
      as C / L\<n\> / R\<n\>. Verify: readouts track knob drags and automation
      playback.
- [x] 5.3 Mono and bypass buttons: mono LED toggle; bypass button state synced
      with the host bypass parameter in both directions. Verify: clicking mono
      folds the image and lights the button; engaging bypass from Cubase's
      control lights the panel button.
- [x] 5.4 `MeterView` custom widget: per-channel RMS fill + peak line + falling
      peak-hold (~2 s) against a dB scale (0/−6/−12/−24/−48), polling
      `MeterState` on a ~30 ms timer; clickable clip LEDs above the bars.
      Verify: steady −10 dBFS sine reads ≈ −10 on both bars; hold marker holds
      then decays; clicking a lit clip LED resets it (completes 4.2).

## 6. Documentation and quality gates

- [x] 6.1 Fill `PRODUCT.md`: what the product is, target user, Round-1 scope
      and feature table, R2–R5 roadmap, constraints (GPL-3, VST3/Cubase,
      Windows-first). Verify: content matches the implemented behavior.
- [x] 6.2 Fill `ARCHITECTURE.md`: components, audio/UI data flow, key decisions
      (D1–D7 summarized), build/run commands, CI and quality-gate setup.
      Verify: the documented build commands reproduce a working build from a
      clean clone.
- [ ] 6.3 Full verification pass: fresh validator run across formats/buffer
      sizes + manual Cubase checklist (mono and stereo instantiation, default
      parameter values, click-free gain automation, pan behavior, mono-sum,
      bypass sync, meter tracking, clip reset). Verify: checklist results
      recorded in the PR description.
- [ ] 6.4 Quality-gate wiring per AGENTS.md: Sonar pipeline wired with the CI
      (Release build under Sonar's build wrapper + SonarCloud scan step gated
      on the `SONAR_TOKEN` secret, `sonar-project.properties` at repo root);
      still open: provision the SonarCloud project and token, wire CodeQL,
      and state the final pipeline status in the PR description. Verify:
      pipeline state (or the blocked note) stated in the PR description.
- [x] 6.5 Add `THIRD-PARTY-NOTICES.md` containing the VST3 SDK MIT license
      text and the VSTGUI BSD-3 notice, referenced from the README. Verify:
      the file is present and referenced before the first release.
