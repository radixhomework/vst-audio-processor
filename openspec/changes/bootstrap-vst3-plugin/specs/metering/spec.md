# metering Specification (delta)

## Purpose

Defines how the plugin measures and exposes the level of the processed output
signal (after mono-sum, gain, and pan) to the user interface.

## ADDED Requirements

### Requirement: Output level measurement
The plugin SHALL continuously measure, per output channel, the peak and RMS
level of the final processed signal. Measurement SHALL run independently of the
editor being open and MUST NOT alter the audio signal.

#### Scenario: Measurement reflects processed signal
- **WHEN** gain is reduced to −6 dB with a steady −6 dBFS input
- **THEN** the reported level per channel measures approximately −12 dBFS

#### Scenario: Runs without UI
- **WHEN** the editor has never been opened and audio has been processing
- **THEN** opening the editor immediately shows levels consistent with the
  recent signal

### Requirement: Peak hold
Each channel SHALL expose a peak-hold value that holds the highest peak for
approximately 2 seconds and then decays toward the current level.

#### Scenario: Transient hold
- **WHEN** a short transient peaks at −3 dBFS
- **THEN** the hold value reads approximately −3 dBFS and remains there for
  about 2 seconds before decaying

### Requirement: Clip detection
The plugin SHALL detect, per output channel, when the processed signal reaches
full scale (a sample magnitude at or above 1.0) and SHALL latch a clip flag
until it is explicitly reset.

#### Scenario: Clip on full-scale output
- **WHEN** the processed output reaches or exceeds full scale on a channel
- **THEN** that channel's clip flag latches on

#### Scenario: Flag resets
- **WHEN** the user resets the clip flag
- **THEN** it clears and stays clear until a new full-scale sample occurs
