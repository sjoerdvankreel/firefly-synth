# Format support
- VST3: supports sample-accurate automation, but no note expressions.
- CLAP: supports sample-accurate automation, supports threadpool, but no modulation (not regular and not polyphonic).

As you might have guessed from the above, it also doesn't do MIDI MPE.<br/>
Preset files (.ffpreset) are shareable between CLAP and VST3.

# Resizing
It's fully resizable by scaling (by dragging the bottom right corner), but it does not react to OS DPI settings.
That means, if you change your DPI settings, you'll have to resize manually. Just once, after that, the size is stored in a user settings file.

# GUI
- Right-click empty space for the undo/redo menu.
- Right-click module number, matrix header, or first matrix column pop-up various context menus to ease editing a bit.

A Knob with a circle in it or a slider with a small dot in it means it can be modulated.
Nearly everything can be automated. Discrete valued parameters are automated per-block (global)
or at-voice-start. Continuous-valued parameters are mostly sample-accurate.

# MIDI
Supports Channel Pressure, Pitch Bend and all 128 CC parameters. You can access the MIDI signals through the routing matrices.
MIDI note number and velocity are also available as modulation sources in the per-voice CV matrix.

# Master module
- MIDI and BPM smoothing parameters. BPM smoothing affects tempo-synced delay lines.
- 3 Auxilliary parameters to be used through the CV routing matrices and automation.
- MIDI-linked modwheel and pitchbend (+ pitchbend range) parameters.<br/>These react to external MIDI sources, have the UI updated etc. This also means they cannot be automated.

# Voice control module
- Oscillator oversampling factor
- Portamento control with optional tempo sync
- Voice pitch (note/cent) parameters affecting all oscillator base pitches

# 10 Per-voice DAHDSR envelopes
- Optional tempo sync
- Smoothing parameter
- 3 envelope modes and 4 slope modes (linear and 3 exponential types)

# 10 Per-voice and 10 global LFOs
- Phase adjustment
- Optional tempo sync
- Smoothing parameter
- Regular repeating or one-shot mode
- Optional 5 horizontal and vertical skewing modes + corresponding control parameters
- Various periodic waveforms + smooth noise, static noise and free-running static noise
