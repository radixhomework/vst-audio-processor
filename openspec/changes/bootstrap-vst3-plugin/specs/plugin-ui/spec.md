# plugin-ui Specification (delta)

## Purpose

Defines the plugin's editor panel: the controls the user manipulates and the
visual feedback the host displays for Round 1.

## ADDED Requirements

### Requirement: Editor panel content
The plugin SHALL provide a custom editor showing: a gain knob, a pan knob, a
mono toggle, a bypass button, and L/R level meters with a dB scale and clip
indicators — covering all Round-1 parameters and metering.

#### Scenario: Opening the editor
- **WHEN** the user opens the plugin UI in the host
- **THEN** the panel shows the gain and pan knobs, mono toggle, bypass button,
  and L/R meters

### Requirement: Resizable editor panel
The editor panel SHALL be resizable by the user (for example by dragging the
host window corner), and the layout SHALL scale proportionally so all
controls, readouts, and meters remain visible and usable. The panel SHALL
enforce minimum and maximum sizes.

#### Scenario: Corner-drag resize
- **WHEN** the user enlarges the editor by dragging the host window corner
- **THEN** the panel grows and every control scales proportionally

#### Scenario: Size bounds
- **WHEN** the user attempts to shrink the panel below the minimum or grow it
  beyond the maximum
- **THEN** the size clamps at the bound

#### Scenario: Content stays usable
- **WHEN** the panel is at its minimum size
- **THEN** knobs, readouts, buttons, meter bars, and clip LEDs are all fully
  visible

### Requirement: Knob readouts
The gain and pan knobs SHALL each display their current value at all times:
gain in dB (displaying "−∞" at minimum), pan as C / L\<n\> / R\<n\>. User
adjustments SHALL go through the host parameters so that knob moves are
recorded by host automation.

#### Scenario: Gain readout
- **WHEN** the gain knob is turned to its minimum
- **THEN** the readout displays "−∞ dB"

#### Scenario: Pan readout format
- **WHEN** the pan knob is set a quarter of the way toward the left
- **THEN** the readout displays "L25"

#### Scenario: Automation visibility
- **WHEN** the user drags a knob during a write-enabled automation pass
- **THEN** the host records the corresponding parameter moves

### Requirement: Mono and bypass buttons
The editor SHALL provide a mono toggle button that lights while engaged, and a
bypass button whose state SHALL stay in sync with the host bypass parameter
whether engaged from the panel or from the host.

#### Scenario: Mono toggle feedback
- **WHEN** the user clicks the mono button
- **THEN** it lights up and the audio folds to mono; clicking again restores
  the stereo pass

#### Scenario: Bypass state sync
- **WHEN** bypass is engaged from the host (not from the panel)
- **THEN** the panel's bypass button shows the engaged state

### Requirement: Meter display
The editor SHALL show one vertical meter bar per output channel, each
displaying an RMS level fill, an instantaneous peak line, and a falling
peak-hold marker against a dB scale (0, −6, −12, −24, −48 marks). Meters SHALL
update smoothly (on the order of 30 times per second) during playback. When
mono-sum is engaged, both bars SHALL show the summed signal.

#### Scenario: Level tracking
- **WHEN** a steady −10 dBFS signal plays through a unity-gain instance
- **THEN** both meter bars read approximately −10 dB

#### Scenario: Mono-summed display
- **WHEN** mono-sum is engaged with distinct left and right signals
- **THEN** the L and R meter bars show identical values

### Requirement: Clip indicator reset
Each meter SHALL carry a clip indicator that lights when the corresponding
clip flag latches, and that resets the flag when clicked.

#### Scenario: Click to reset
- **WHEN** a channel has clipped and the user clicks its lit clip indicator
- **THEN** the indicator goes dark and remains dark until a new clip occurs
