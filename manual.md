# Firefly Synth
A semi-modular polyphonic synthesizer plugin with different oscillator and effect types,<br/>
2 osc-to-osc routing matrices and both global and per-voice audio and cv routing matrices.

## File and plugin format
- Ffpreset files are shareable between CLAP and VST3.
- VST3: does sample accurate automation, no note expressions.
- CLAP: does sample accurate automation, does threadpool, no modulation, no polyphonic modulation.<br/>
However, this synth is quite capable of per-voice modulation on it's own.
- MIDI: does PitchBend, ChannelPressure, all CC parameters. No MPE support.

## UI
A knob with a circle in it or a slider with a small dot in it means it can be modulated by the CV matrices.<br/>
There is no theming support.

### Resizing
It's fully resizable by scaling (by dragging the bottom right corner), but it does not react to OS DPI settings.<br/>
That means, if you change your DPI settings, you'll have to resize manually.<br/>
Just once, after that, the size is stored in a user settings file.

### Context menus
- Right-click a parameter to show the host menu.
- Right-click some empty space to show the undo/redo menu.
- Right-click a matrix header to show clear/tidy matrix options.
- Right-click a module header to show copy/clear/swap etc options.
- Right-click the first column in a matrix to show matrix manipulation options.
