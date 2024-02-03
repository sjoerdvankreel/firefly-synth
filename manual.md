# Firefly Synth

A semi-modular polyphonic synthesizer plugin with different oscillator and effect types,<br/>
2 osc-to-osc routing matrices and both global and per-voice audio and cv routing matrices.

## File and plugin format

- Ffpreset files are shareable between CLAP and VST3.
- VST3: does sample accurate automation, no note expressions.
- CLAP: does sample accurate automation, does threadpool, no modulation, no polyphonic modulation.<br/>
However, this synth is quite capable of per-voice modulation on it's own.
- MIDI: does PitchBend, ChannelPressure, all CC parameters. No MPE support.

## UI resizing

It's fully resizable by scaling (by dragging the bottom right corner), but it does not react to OS DPI settings.<br/>
That means, if you change your DPI settings, you'll have to resize manually.
Just once, after that, the size is stored in a user settings file.
