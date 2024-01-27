# About
A semi-modular software synthesizer plugin.
It's basically [InfernalSynth](https://github.com/sjoerdvankreel/infernal-synth)'s big brother.

![Screenshot](static/screenshot.png)

<img align="left" alt="VST logo" src="static/vst_logo.png">
VST is a trademark of Steinberg Media Technologies GmbH,
registered in Europe and other countries.
<br clear="left"/>

# System requirements & supported environments
- 64-bit cpu with AVX support.
- Linux: tested on Ubuntu 22 only. Linux support is highly experimental.<br/>That being said, and unlike InfernalSynth, it actually seems to work.
- Windows: tested on Windows 10. 7+ should work but you might need this:<br/>[https://learn.microsoft.com/en-US/cpp/windows/latest-supported-vc-redist?view=msvc-170](https://learn.microsoft.com/en-US/cpp/windows/latest-supported-vc-redist?view=msvc-170).
- Explicitly supported hosts: Reaper, Bitwig, FLStudio. Please use a recent version.
- Explicitly NOT supported hosts: Waveform (I have too many problems with it), Cakewalk and Renoise. I really want to support renoise, but see here: https://forum.renoise.com/t/possible-probable-bug-w-r-t-vst3-parameter-flushing/70684. I suspect cakewalk suffers the same problem.
- All other hosts, you'll just have to try and see.


