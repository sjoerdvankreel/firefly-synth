# About

A semi-modular software synthesizer and fx plugin.
It's basically [InfernalSynth](https://github.com/sjoerdvankreel/infernal-synth)'s big brother.

- Changelog: [changelog.md](changelog.md).
- Parameter reference: [param_reference.html](https://htmlpreview.github.io/?https://github.com/sjoerdvankreel/firefly-synth/blob/main/param_reference.html).
- KVR: [https://www.kvraudio.com/product/firefly-synth-by-sjoerdvankreel](https://www.kvraudio.com/product/firefly-synth-by-sjoerdvankreel)
- Manual: [https://github.com/sjoerdvankreel/firefly-synth/blob/main/manual.md](https://github.com/sjoerdvankreel/firefly-synth/blob/main/manual.md)

# Screenshot

See below for some built-in themes.

![Screenshot](static/screenshot_firefly_dark.png)

<table>
  <tr>
    <td><img alt="CLAP logo" src="static/clap_logo.png"/></td>
    <td><a href="https://github.com/free-audio/clap">https://github.com/free-audio/clap</a></td>
    <td><img alt="VST logo" src="static/vst_logo.png"/></td>
    <td>VST is a trademark of Steinberg Media Technologies GmbH, registered in Europe and other countries.</td>
  </tr>
</table>

# What does it sound like?

Pretty much like InfernalSynth. I reused most of the algorithms, although some of them got upgraded. In particular,
the waveshaper becomes a full-blown distortion module, the Karplus-Strong oscillator has some extra knobs to play
around with, there's a new CV-to-CV mod matrix and last-but-not-least, it can do actual (Chowning-style) FM synthesis.

See also [demos](https://github.com/sjoerdvankreel/firefly-synth/tree/main/demos) for project files.
Not all of these sounds are presets, but if you want you can extract the other ones from there.

Demo tunes:
- Demo track 1: [demo_track_1.mp3](https://github.com/sjoerdvankreel/firefly-synth-storage/raw/main/render/demo_track_1.mp3)
- Demo track 2: [demo_track_2.mp3](https://github.com/sjoerdvankreel/firefly-synth-storage/raw/main/render/demo_track_2.mp3)
- Demo track 3: [demo_track_3.mp3](https://github.com/sjoerdvankreel/firefly-synth-storage/raw/main/render/demo_track_3.mp3)

Other:
- FX Demo: [fx_demo_reaper_clap.mp3](https://github.com/sjoerdvankreel/firefly-synth-storage/raw/main/render/fx_demo_reaper_clap.mp3)
- Noise generator demo: [stereo_noise_reaper_clap.mp3](https://github.com/sjoerdvankreel/firefly-synth-storage/raw/main/render/stereo_noise_reaper_clap.mp3)
- Distortion modulation demo: [distortion_modulation.mp3](https://github.com/sjoerdvankreel/firefly-synth-storage/raw/main/render/distortion_modulation.mp3)
- I Love Distortion + Global Unison: [i_love_distortion_global_unison.mp3](https://github.com/sjoerdvankreel/firefly-synth-storage/raw/main/render/i_love_distortion_global_unison.mp3)

Presets:
- Big Pad (Preset): [big_pad.mp3](https://github.com/sjoerdvankreel/firefly-synth-storage/raw/main/render/big_pad.mp3)
- Big Pad 2 (Preset): [big_pad_2.mp3](https://github.com/sjoerdvankreel/firefly-synth-storage/raw/main/render/big_pad_2.mp3)
- Big Pad 3 (Preset): [big_pad_2.mp3](https://github.com/sjoerdvankreel/firefly-synth-storage/raw/main/render/big_pad_3.mp3)
- Mono Lead (Preset): [mono_lead.mp3](https://github.com/sjoerdvankreel/firefly-synth-storage/raw/main/render/mono_lead_reaper_clap.mp3)
- Infernal Acid (Preset): [infernal_acid.mp3](https://github.com/sjoerdvankreel/firefly-synth-storage/raw/main/render/infernal_acid.mp3)
- FM Weirdness (Preset): [fm_weirdness.mp3](https://github.com/sjoerdvankreel/firefly-synth-storage/raw/main/render/fm_weirdness.mp3)
- AM Bells (Preset): [am_bells_reaper_clap.mp3](https://github.com/sjoerdvankreel/firefly-synth-storage/raw/main/render/am_bells_reaper_clap.mp3)
- AM/FM Unison Pad (Preset): [am_fm_unison_pad.mp3](https://github.com/sjoerdvankreel/firefly-synth-storage/raw/main/render/am_fm_unison_pad.mp3)
- Karplus-Strong (Preset): [karplus_strong_reaper_clap.mp3](https://github.com/sjoerdvankreel/firefly-synth-storage/raw/main/render/karplus_strong_reaper_clap.mp3)
- I Love Distortion (Preset): [i_love_distortion_reaper_clap.mp3](https://github.com/sjoerdvankreel/firefly-synth-storage/raw/main/render/i_love_distortion_reaper_clap.mp3)
- Fun With Hard Sync And FM (Preset): [fun_with_hard_sync_and_fm.mp3](https://github.com/sjoerdvankreel/firefly-synth-storage/raw/main/render/fun_with_hard_sync_and_fm.mp3)

# System requirements and supported environments
- 64-bit cpu with AVX support.
- Linux: tested on Ubuntu 22 only. Known to work on Fedora, too.
- Windows: tested on Windows 10. Windows 7+ should work but you might need this:<br/>[https://learn.microsoft.com/en-US/cpp/windows/latest-supported-vc-redist?view=msvc-170](https://learn.microsoft.com/en-US/cpp/windows/latest-supported-vc-redist?view=msvc-170).
- Explicitly supported hosts: Reaper, Bitwig, FLStudio. Please use a recent version.
- Explicitly NOT supported hosts:<br/>Waveform (too many problems), Renoise (bug), Cakewalk (same bug as renoise, probably).
- Renoise support waits for this:<br/>https://forum.renoise.com/t/possible-probable-bug-w-r-t-vst3-parameter-flushing/70684.
- All other hosts, you'll just have to try and see.

# Download and install
- Windows VST3:<br/>[https://github.com/sjoerdvankreel/firefly-synth-storage/raw/main/release/firefly_synth_1.5.0_windows_vst3.zip](https://github.com/sjoerdvankreel/firefly-synth-storage/raw/main/release/firefly_synth_1.5.0_windows_vst3.zip)
- Windows VST3 Fx:<br/>[https://github.com/sjoerdvankreel/firefly-synth-storage/raw/main/release/firefly_synth_fx_1.5.0_windows_vst3.zip](https://github.com/sjoerdvankreel/firefly-synth-storage/raw/main/release/firefly_synth_fx_1.5.0_windows_vst3.zip)
- Windows CLAP:<br/>[https://github.com/sjoerdvankreel/firefly-synth-storage/raw/main/release/firefly_synth_1.5.0_windows_clap.zip](https://github.com/sjoerdvankreel/firefly-synth-storage/raw/main/release/firefly_synth_1.5.0_windows_clap.zip)
- Windows CLAP Fx:<br/>[https://github.com/sjoerdvankreel/firefly-synth-storage/raw/main/release/firefly_synth_fx_1.5.0_windows_clap.zip](https://github.com/sjoerdvankreel/firefly-synth-storage/raw/main/release/firefly_synth_fx_1.5.0_windows_clap.zip)

Extract the zipped folder and copy it (the entire folder, not just the .dll/.so!) to your default VST3/CLAP folder. On Windows, this is "C:\Program Files\Common Files\\[VST3/CLAP]".

There are also Linux binaries over here: [https://github.com/sjoerdvankreel/firefly-synth-storage/tree/main/release](https://github.com/sjoerdvankreel/firefly-synth-storage/tree/main/release),
but they are built without AVX support, so it's probably better to build from source.

# Build from source
- git clone --recursive [this-project]
- Linux: scripts/build.sh [Debug|RelWithDebInfo|Release]
- Windows: scripts/build.bat [Debug|RelWithDebInfo|Release]

You'll need CMake and a very recent c++ compiler. The final output ends up in the /dist folder. <br/>
Like noted above, the default Linux build does *not* enable AVX.
To fix that, you'll have to edit [https://github.com/sjoerdvankreel/firefly-synth/blob/main/plugin_base/cmake/plugin_base.config.cmake](https://github.com/sjoerdvankreel/firefly-synth/blob/main/plugin_base/cmake/plugin_base.config.cmake).

# Dependencies
- CLAP SDK: [https://github.com/free-audio/clap](https://github.com/free-audio/clap)
- JUCE: [https://github.com/juce-framework/JUCE](https://github.com/juce-framework/JUCE)
- Steinberg VST3 SDK: [https://github.com/steinbergmedia/vst3sdk](https://github.com/steinbergmedia/vst3sdk)
- Readerwriterqueue: [https://github.com/cameron314/readerwriterqueue](https://github.com/cameron314/readerwriterqueue)

# Credits
- Special thanks to [oddyolynx](https://github.com/tank-trax) for supporting the project early-on.
- Special thanks to [BaconPaul](https://baconpaul.org/) for helping me out with CLAP support.
- Special thanks to host vendors who have set me up with an NFR license.
- Infernal theme artwork by Dante Cyberman: [https://www.deviantart.com/dantecyberman](https://www.deviantart.com/dantecyberman).
- Infernal theme artwork by Vlad Darkness Art: [https://www.deviantart.com/vladdarknessart](https://www.deviantart.com/vladdarknessart).
- Contains soft clippers by [Sean Enderby and Zlatko Baracskai](https://dafx.de/paper-archive/2012/papers/dafx12_submission_45.pdf).
- Contains a slightly adapted implementation of [Jezar's Freeverb](https://github.com/sinshu/freeverb).
- Contains a slightly adapted implementation of the [Karplus-Strong algorithm](https://blog.demofox.org/2016/06/16/synthesizing-a-pluked-string-sound-with-the-karplus-strong-algorithm).
- Contains a verbatim implementation of [Andrew Simper's state variable filter equations](https://cytomic.com/files/dsp/SvfLinearTrapOptimised2.pdf).
- Contains a verbatim implementation of [Moorer's DSF algorithm as described by Burkhard Reike](https://www.verklagekasper.de/synths/dsfsynthesis/dsfsynthesis.html).

# Screenshot FX version

![Screenshot](static/screenshot_fx_firefly_dark.png)

# Themes

<table>
  <tr>
    <td><img src="static/screenshot_firefly_dark.png"/></td>
    <td><img src="static/screenshot_firefly_light.png"/></td>
    <td><img src="static/screenshot_firefly_flat.png"/></td>
  </tr>
  <tr>
    <td><img src="static/screenshot_infernal_dark.png"/></td>
    <td><img src="static/screenshot_infernal_light.png"/></td>
    <td><img src="static/screenshot_infernal_flat.png"/></td>
  </tr>
  <tr>
    <td><img src="static/screenshot_mauve_dark.png"/></td>
    <td><img src="static/screenshot_mauve_light.png"/></td>
    <td><img src="static/screenshot_mauve_flat.png"/></td>
  </tr>
  <tr>
    <td><img src="static/screenshot_cyan_dark.png"/></td>
    <td><img src="static/screenshot_cyan_light.png"/></td>
    <td><img src="static/screenshot_cyan_flat.png"/></td>
  </tr>
  <tr>
    <td><img src="static/screenshot_orange_dark.png"/></td>
    <td><img src="static/screenshot_orange_light.png"/></td>
    <td><img src="static/screenshot_orange_flat.png"/></td>
  </tr>
  <tr>
    <td><img src="static/screenshot_cool_blue_dark.png"/></td>
    <td><img src="static/screenshot_cool_blue_light.png"/></td>
    <td><img src="static/screenshot_cool_blue_flat.png"/></td>
  </tr>
</table>
