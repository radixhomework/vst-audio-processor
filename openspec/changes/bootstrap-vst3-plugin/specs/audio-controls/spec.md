# audio-controls Specification (delta)

## Purpose

Defines the audible processing behavior of the Round-1 chain — mono-sum, gain,
and pan — including their parameter ranges, defaults, laws, and the fixed stage
order.

## ADDED Requirements

### Requirement: Processing chain order
The audio signal SHALL flow through the processing stages in a fixed order:
mono-sum, then gain, then pan. The behavior requirements below assume this
order.

#### Scenario: Combined stages
- **WHEN** mono-sum is engaged and gain is set to −6 dB on a stereo instance
  fed full-scale correlated input (L = R at 0 dBFS)
- **THEN** each output channel carries the same mono signal at −6 dBFS

### Requirement: Gain control
The gain parameter SHALL adjust the output level across a range from −∞ (full
mute) to +12 dB, with 0 dB (unity) as default and 0.1 dB resolution. Parameter
changes SHALL be applied as a smooth ramp across each audio buffer so that no
zipper noise or discontinuity is audible.

#### Scenario: Unity by default
- **WHEN** a new plugin instance is created with centered pan and mono-sum off
- **THEN** gain is 0 dB and the output level equals the input level

#### Scenario: Full attenuation
- **WHEN** gain is set to its minimum value
- **THEN** the output is digital silence

#### Scenario: Maximum boost
- **WHEN** gain is set to +12 dB and a −12 dBFS sine is fed to the input
- **THEN** the output peak measures approximately 0 dBFS (within measurement
  tolerance)

#### Scenario: Click-free automation
- **WHEN** the gain parameter is automated continuously during playback
- **THEN** the level changes smoothly with no audible clicks or steps

### Requirement: Pan/balance control
The pan parameter SHALL position the signal across the stereo field, from hard
left (−100) through center (0, default) to hard right (+100). On stereo
instances it acts as a balance control: moving toward one side attenuates the
opposite channel following a constant-power taper while the favored channel
remains at unity; centered leaves both channels at unity. On mono instances the
control SHALL have no effect on the single-channel signal.

#### Scenario: Centered
- **WHEN** pan is at 0 on a stereo instance
- **THEN** both output channels equal their corresponding input channels

#### Scenario: Hard left
- **WHEN** pan is set to −100 on a stereo instance
- **THEN** the left output equals the left input at unity and the right output
  is silent

#### Scenario: Constant-power taper
- **WHEN** pan is moved from center toward hard left while a constant-level
  stereo signal plays
- **THEN** the overall loudness stays approximately constant (the fading
  channel follows a constant-power curve, not a linear one)

#### Scenario: Mono instance
- **WHEN** the plugin runs with the mono arrangement and pan is moved
- **THEN** the output signal is unaffected

### Requirement: Mono-sum switch
The mono-sum parameter SHALL, when engaged on a stereo instance, fold the left
and right channels into a single mono signal reproduced identically on both
output channels. The sum SHALL be attenuated by 6 dB so that a full-scale
correlated input does not clip. The switch SHALL default to off and SHALL have
no effect on mono instances.

#### Scenario: Summed image
- **WHEN** mono-sum is engaged on a stereo instance fed distinct left and right
  signals
- **THEN** both output channels carry the identical summed mono signal

#### Scenario: No clipping on correlated full-scale input
- **WHEN** mono-sum is engaged and L = R at full scale
- **THEN** the output does not exceed full scale

#### Scenario: Off by default
- **WHEN** a new plugin instance is created
- **THEN** mono-sum is off and the stereo image passes through unaltered
