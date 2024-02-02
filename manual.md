# Firefly Synth

A semi-modular synth plugin with various options including subtractive, DSF and FM.<br/>
For a more detailed overview, see the parameter reference document.

# General

## Plugin format

- VST3: does sample accurate automation, no note expressions.
- CLAP: does sample accurate automation, does threadpool, no modulation, no polyphonic modulation.<br/>
However, this synth is quite capable of per-voice modulation on it's own.
- MIDI: does PitchBend, ChannelPressure, all CC parameters. No MPE support.

## File format

.ffpreset files are shareable between CLAP and VST3.<br/>
Those are regular JSON files with some checksums built-in.<br/>
You won't be able to edit them (less you break the checksum).

## UI

# Resizing

It's fully resizable by scaling (by dragging the bottom right corner), but it does not react to OS DPI settings.<br/>
That means, if you change your DPI settings, you'll have to resize manually.<br/>
Just once, after that, the size is stored in a user settings file.

varken8

EFFEKIJKEN OFHET WERKT

this is a manual

JAHALLO