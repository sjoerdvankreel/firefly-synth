# About
A semi-modular software synthesizer plugin.
It's basically [InfernalSynth](https://github.com/sjoerdvankreel/infernal-synth)'s big brother.

![Screenshot](static/screenshot.png)

<img align="left" alt="VST logo" src="static/vst_logo.png">
VST is a trademark of Steinberg Media Technologies GmbH,
registered in Europe and other countries.
<br clear="left"/>

# System requirements and supported environments
- 64-bit cpu with AVX support.
- Linux: tested on Ubuntu 22 only. Linux support is highly experimental.<br/>That being said, and unlike InfernalSynth, it actually seems to work.
- Windows: tested on Windows 10. 7+ should work but you might need this:<br/>[https://learn.microsoft.com/en-US/cpp/windows/latest-supported-vc-redist?view=msvc-170](https://learn.microsoft.com/en-US/cpp/windows/latest-supported-vc-redist?view=msvc-170).
- Explicitly supported hosts: Reaper, Bitwig, FLStudio. Please use a recent version.
- Explicitly NOT supported hosts:<br/>Waveform (too many problems), Renoise (bug), Cakewalk (same bug as renoise, probably).
- Renoise support waits for this:<br/>https://forum.renoise.com/t/possible-probable-bug-w-r-t-vst3-parameter-flushing/70684.
- All other hosts, you'll just have to try and see.

# Download and install
- Windows VST3:<br/>[https://github.com/sjoerdvankreel/firefly-synth-storage/raw/main/release/firefly_synth_1_windows_vst3.zip](https://github.com/sjoerdvankreel/firefly-synth-storage/raw/main/release/firefly_synth_1_windows_vst3.zip)
- Windows CLAP:<br/>[https://github.com/sjoerdvankreel/firefly-synth-storage/raw/main/release/firefly_synth_1_windows_clap.zip](https://github.com/sjoerdvankreel/firefly-synth-storage/raw/main/release/firefly_synth_1_windows_clap.zip)

Extract the zipped folder and copy it (the entire folder, not just the .dll/.so!) to your default VST3/CLAP folder. On Windows, this is "C:\Program Files\Common Files\\[VST3/CLAP]".

There are also Linux binaries over here: [https://github.com/sjoerdvankreel/firefly-synth-storage/tree/main/release](https://github.com/sjoerdvankreel/firefly-synth-storage/tree/main/release),
but they are built without AVX support, and I have also no idea how well they behave on non-Ubuntu like distro's. It's probably better to build from source,
but as I only built on Ubuntu 22, there's bound to be compiler errors on very different toolchains.

# todo
# todo linux note
# todo project status

# Download and install
- Binary releases can be found here: