# Format support
- VST3: supports sample-accurate automation, but no note expressions.
- CLAP: supports sample-accurate automation, supports threadpool, but no modulation (not regular and not polyphonic).

As you might have guessed from the above, it also doesn't do MIDI MPE.

# Resizing
It's fully resizable by scaling (by dragging the bottom right corner), but it does not react to OS DPI settings.
That means, if you change your DPI settings, you'll have to resize manually. Just once, after that, the size is stored in a user settings file.

# MIDI
Supports Channel Pressure, Pitch Bend and all 128 CC parameters. You can access the MIDI signals through the routing matrices.
MIDI note number and velocity are also available as modulation sources in the per-voice CV matrix.

# Master module
- MIDI and BPM smoothing parameters. BPM smoothing affects tempo-synced delay lines.
- 3 Auxilliary parameters to be used through the CV routing matrices and automation.
- MIDI-linked modwheel and pitchbend (+ pitchbend range) parameters.<br/>These react to external MIDI sources, have the UI updated etc. This also means they cannot be automated.

# 10 Per-voice DAHDSR envelopes
- Optional tempo sync
- Smoothing parameter
- 3 envelope modes and 4 slope modes (linear and 3 exponential types)

# 10 Per-voice and 10 global LFOs
- Phase adjustment
- Smoothing parameter
- Optional tempo sync
- Regular repeating or one-shot mode
- Optional 5 horizontal and vertical skewing modes + corresponding control parameters
- Various periodic waveforms + smooth noise, static noise and free-running static noise
