# Format support
- VST3: supports sample-accurate automation, but no note expressions.
- CLAP: supports sample-accurate automation, supports threadpool, but no modulation (not regular and not polyphonic).

As you might have guessed from the above, it also doesn't do MIDI MPE.

# Resizing
It's fully resizable by scaling (by dragging the bottom right corner), but it does not react to OS DPI settings.
That means, if you change your DPI settings, you'll have to resize manually. Just once, after that, the size is stored in a user settings file.

