# plugin-core Specification (delta)

## Purpose

Defines the host-facing contract of the plugin: how it identifies itself, which
channel layouts the host can instantiate, which sample formats it processes,
how host bypass behaves, and the real-time obligations of the processing core.

## ADDED Requirements

### Requirement: Plugin identity
The plugin SHALL identify itself to the host with the name "Audio Processor"
and the vendor "Radixhomework", so it is listed and labeled consistently in the
host's plug-in browser and insert slots.

#### Scenario: Plugin listed in host
- **WHEN** the host scans and loads the plugin
- **THEN** it appears as "Audio Processor" from vendor "Radixhomework"

### Requirement: Mono and stereo bus configurations
The plugin SHALL support exactly two channel configurations: mono (1 input, 1
output channel) and stereo (2 input, 2 output channels). The host SHALL be able
to instantiate the plugin on either layout; no other layouts (including
sidechain inputs) are offered in this version.

#### Scenario: Instantiation on a mono track
- **WHEN** the host requests the mono 1-in/1-out arrangement
- **THEN** the plugin activates processing with one input and one output channel

#### Scenario: Instantiation on a stereo track
- **WHEN** the host requests the stereo 2-in/2-out arrangement
- **THEN** the plugin activates processing with two input and two output channels

#### Scenario: Unsupported layouts are not offered
- **WHEN** the host queries supported arrangements (e.g. surround or sidechain)
- **THEN** only the mono and stereo arrangements are reported as supported

### Requirement: Sample formats and rates
The plugin SHALL process 32-bit and 64-bit float audio at sample rates from
44.1 kHz to 192 kHz and at any host buffer size.

#### Scenario: Typical session
- **WHEN** the host activates the plugin at 48 kHz, 64-bit float, 128-sample
  buffers
- **THEN** audio processes normally with no errors reported to the host

### Requirement: Host bypass
The plugin SHALL support the VST3 standard bypass parameter. While bypass is
engaged, the audio input SHALL be passed to the output unmodified, and the
gain/pan/mono controls SHALL have no effect on the audio.

#### Scenario: Audio passes through while bypassed
- **WHEN** bypass is engaged and a test signal is fed to the input
- **THEN** the output matches the input sample-for-sample

#### Scenario: Controls are inert while bypassed
- **WHEN** bypass is engaged and the gain parameter is moved to its minimum
- **THEN** the output still matches the input sample-for-sample

### Requirement: Real-time-safe processing
Once activated, the audio processing path MUST NOT perform blocking operations
(memory allocation, file access, lock acquisition) that could violate the
host's real-time constraints.

#### Scenario: Validator run
- **WHEN** the plugin is exercised by the VST3 validator across sample formats
  and buffer sizes
- **THEN** it completes processing without real-time violations or host errors
