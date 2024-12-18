"This wouldn't be FOSSPicks without a new open source software synthesizer, and this time it's Firefly Synth's turn � which isn't a synth designed to recreate the sound of flying insects.
Instead, it's capable of producing a huge range of powerfully complex sounds, from beautifully warm analog strings and brass-like melodies to percussion and eardrum piercing leads.
It does all this from a user interface totally unlike any other software synth, replacing skeuomorphism with practical and instant access to dozens of its features.
The only downside is that there are so many features, it's difficult to cram them all into the user interface, making it all look more complicated than it really is."

&ndash; FOSSPicks 282, May 2024

# Download

Downloads have moved to the [releases](https://github.com/sjoerdvankreel/firefly-synth/releases) section.

MacOS Note: if you get a warning like "archive damaged" or "failed to open" etc, it's probably this:
[https://syntheway.com/fix-au-vst-vst3-macos.htm](https://syntheway.com/fix-au-vst-vst3-macos.htm)

# About

A semi-modular software synthesizer and fx plugin.
It's basically [InfernalSynth](https://github.com/sjoerdvankreel/infernal-synth)'s big brother.

- Changelog: [changelog.md](changelog.md).
- Parameter reference: [param_reference.html](https://htmlpreview.github.io/?https://github.com/sjoerdvankreel/firefly-synth/blob/main/param_reference.html).
- KVR: [https://www.kvraudio.com/product/firefly-synth-by-sjoerdvankreel](https://www.kvraudio.com/product/firefly-synth-by-sjoerdvankreel)
- Manual: [https://github.com/sjoerdvankreel/firefly-synth/blob/main/manual.md](https://github.com/sjoerdvankreel/firefly-synth/blob/main/manual.md)

# Legal

<table>
  <tr>
    <td><img alt="CLAP logo" src="static/clap_logo.png"/></td>
    <td><a href="https://github.com/free-audio/clap">https://github.com/free-audio/clap</a></td>
    <td><img alt="VST logo" src="static/vst_logo.png"/></td>
    <td>VST is a trademark of Steinberg Media Technologies GmbH, registered in Europe and other countries.</td>
  </tr>
</table>

# Screenshots / Demo videos

Realtime modulation animation videos:

- [mseg_demo.mp4](https://github.com/sjoerdvankreel/firefly-synth-storage/raw/main/video/mseg_demo.mp4)
- [big_pad_2_visual_demo.mp4](https://github.com/sjoerdvankreel/firefly-synth-storage/raw/main/video/big_pad_2_visual_demo.mp4)
- [arp_demo_2_reaper_clap.mp4](https://github.com/sjoerdvankreel/firefly-synth-storage/raw/main/video/arp_demo_2_reaper_clap.mp4)
- [arp_demo_1_renoise_vst3.mp4](https://github.com/sjoerdvankreel/firefly-synth-storage/raw/main/video/arp_demo_1_renoise_vst3.mp4)
- [saw_to_dsf_distortion_demo.mp4](https://github.com/sjoerdvankreel/firefly-synth-storage/raw/main/video/saw_to_dsf_distortion_demo.mp4)
- [modulation_visualization_demo.mp4](https://github.com/sjoerdvankreel/firefly-synth-storage/raw/main/video/modulation_visualization_demo.mp4)

<table>
<tr>
<td><img alt="Screenshot" src="static/screenshot_hot_dark.png"></td>
<td><img alt="Screenshot" src="static/screenshot_hot_light.png"></td>
</tr>
<tr>
<td><img alt="Screenshot" src="static/screenshot_hot_dark_fx.png"></td>
<td><img alt="Screenshot" src="static/screenshot_hot_light_fx.png"></td>
</tr>
<tr>
<td><img alt="Screenshot" src="static/screenshot_cold_dark.png"></td>
<td><img alt="Screenshot" src="static/screenshot_cold_light.png"></td>
</tr>
<tr>
<td><img alt="Screenshot" src="static/screenshot_cold_dark_fx.png"></td>
<td><img alt="Screenshot" src="static/screenshot_cold_light_fx.png"></td>
</tr>
</table>

# Install
Extract the zipped folder and copy/replace it (the entire folder, not just the .dll/.so/.dylib!) to your VST3/CLAP folder:

- Windows: https://helpcenter.steinberg.de/hc/en-us/articles/115000177084-VST-plug-in-locations-on-Windows (replace VST3 by CLAP for clap)
- Mac: https://helpcenter.steinberg.de/hc/en-us/articles/115000171310-VST-plug-in-locations-on-Mac-OS-X-and-macOS (replace VST3 by CLAP for clap)
- Linux: ~/.vst3 or ~/.clap

# System requirements and supported environments
- Windows: 64-bit (X64) cpu with AVX support. Tested on Windows 11. Needs Windows 10+.
- Mac: 64-bit (X64) cpu for Intel-based Macs. 64-bit ARM cpu for Apple Silicon. Universal binaries are provided. Tested on MacOS 14, minimum required is 10.15.
- Linux: 64-bit (X64) cpu. Provided binaries do not require AVX support, but you might want to build with march=native for better performance. Tested on Ubuntu 22, minimum required is 18. Needs glibc 2.25+. Known to work on Fedora and Mint, too.
- Explicitly supported hosts: Reaper, Bitwig, FLStudio, Waveform 13+. Please use a recent version.
- Explicitly NOT supported hosts:<br/>Renoise (bug), Cakewalk (probably same as this: https://forum.renoise.com/t/possible-probable-bug-w-r-t-vst3-parameter-flushing/70684/9).
- All other hosts, you'll just have to try and see.

Waveform note:<br/>
It is necessary to re-scan plugins after updating.

FLStudio note:<br/>
Firefly has a relatively large per-block overhead.
If you notice large spikes in CPU usage, try enabling fixed-size buffers.

Renoise note:<br/>
Still not really supported. It will work on 3.4.4+, but updating the plugin version will cause loss of automation data.
See https://forum.renoise.com/t/saved-automation-data-does-not-respect-vst3s-parameter-id/68461.

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
- Hardcore kicks take 1: [hardcore_kicks_take_1.mp3](https://github.com/sjoerdvankreel/firefly-synth-storage/raw/main/render/hardcore_kicks_take_1.mp3)
- Hardcore kicks take 2: [hardcore_kicks_take_2.mp3](https://github.com/sjoerdvankreel/firefly-synth-storage/raw/main/render/hardcore_kicks_take_2.mp3)
- Arpeggiator demo tune: [arp_demo_tune_reaper_clap](https://github.com/sjoerdvankreel/firefly-synth-storage/raw/main/render/arp_demo_tune_reaper_clap.mp3)
- Fun with actual supersaw (Supersaw preset): [fun_with_actual_supersaw.mp3](https://github.com/sjoerdvankreel/firefly-synth-storage/raw/main/render/fun_with_actual_supersaw.mp3)
- Fun with rave synth (Rave Synth Preset + Big Pad 2 Preset): [fun_with_rave_synth.mp3](https://github.com/sjoerdvankreel/firefly-synth-storage/raw/main/render/fun_with_rave_synth.mp3)
- Fun with supersaw (Detuned saw + more bells + kick 7 + closed hat 4 + bass 4): [fun_with_supersaw.mp3](https://github.com/sjoerdvankreel/firefly-synth-storage/raw/main/render/fun_with_supersaw.mp3)
- Look ma, all FM and no filters (FM Bass Preset + FM Clap + FM FX + FM Bells 2/3): [look_ma_all_fm_and_no_filters.mp3](https://github.com/sjoerdvankreel/firefly-synth-storage/raw/main/render/look_ma_all_fm_and_no_filters.mp3)
- Renoise test (sort of trance melody hard edit) (Rave Synth Preset): [renoise_test_sort_of_trance_melody_hard_edit.mp3](https://github.com/sjoerdvankreel/firefly-synth-storage/raw/main/render/renoise_test_sort_of_trance_melody_hard_edit.mp3)
- Renoise test (sort of goa trance) (Trance Pad 2 Preset + Goa Bass Preset + Some more): [renoise_test_sort_of_goa_trance.mp3](https://github.com/sjoerdvankreel/firefly-synth-storage/raw/main/render/renoise_test_sort_of_goa_trance.mp3)
- Downtempo ambient psy (Bass 3 Preset + Closed Hat 3 + Distorted Bells 2 + FM Bells + Kick 6 + Open Hat 3): [downtempo_ambient_psy.mp3](https://github.com/sjoerdvankreel/firefly-synth-storage/raw/main/render/downtempo_ambient_psy.mp3)

Other:
- FX Demo: [fx_demo_reaper_clap.mp3](https://github.com/sjoerdvankreel/firefly-synth-storage/raw/main/render/fx_demo_reaper_clap.mp3)
- Arpeggiator demo 1: [arp_demo_1_renoise_vst3](https://github.com/sjoerdvankreel/firefly-synth-storage/raw/main/render/arp_demo_1_renoise_vst3.mp3)
- Arpeggiator demo 2: [arp_demo_2_reaper_clap](https://github.com/sjoerdvankreel/firefly-synth-storage/raw/main/render/arp_demo_2_reaper_clap.mp3)
- DSF distortion: [saw_to_dsf_distortion_demo.mp3](https://github.com/sjoerdvankreel/firefly-synth-storage/raw/main/render/saw_to_dsf_distortion_demo.mp3)
- Noise generator demo: [stereo_noise_reaper_clap.mp3](https://github.com/sjoerdvankreel/firefly-synth-storage/raw/main/render/stereo_noise_reaper_clap.mp3)
- Distortion modulation demo: [distortion_modulation.mp3](https://github.com/sjoerdvankreel/firefly-synth-storage/raw/main/render/distortion_modulation.mp3)
- I Love Distortion + Global Unison: [i_love_distortion_global_unison.mp3](https://github.com/sjoerdvankreel/firefly-synth-storage/raw/main/render/i_love_distortion_global_unison.mp3)
- Visual modulation demo [visual_modulation_demo_no_automation_reaper_clap.mp3](https://github.com/sjoerdvankreel/firefly-synth-storage/raw/main/render/visual_modulation_demo_no_automation_reaper_clap.mp3)
- Renoise test (sort of trance melody) (Trance Pad Preset): [renoise_test_sort_of_trance_melody.mp3](https://github.com/sjoerdvankreel/firefly-synth-storage/raw/main/render/renoise_test_sort_of_trance_melody.mp3)
- Renoise test (yet another acid line) (Yet Another Acid Preset): [renoise_test_yet_another_acid_line.mp3](https://github.com/sjoerdvankreel/firefly-synth-storage/raw/main/render/renoise_test_yet_another_acid_line.mp3)

Presets:
- Big Pad (Preset): [big_pad.mp3](https://github.com/sjoerdvankreel/firefly-synth-storage/raw/main/render/big_pad.mp3)
- Big Pad 2 (Preset): [big_pad_2.mp3](https://github.com/sjoerdvankreel/firefly-synth-storage/raw/main/render/big_pad_2.mp3)
- Big Pad 3 (Preset): [big_pad_3.mp3](https://github.com/sjoerdvankreel/firefly-synth-storage/raw/main/render/big_pad_3.mp3)
- Mono Lead (Preset): [mono_lead.mp3](https://github.com/sjoerdvankreel/firefly-synth-storage/raw/main/render/mono_lead_reaper_clap.mp3)
- Infernal Acid (Preset): [infernal_acid.mp3](https://github.com/sjoerdvankreel/firefly-synth-storage/raw/main/render/infernal_acid.mp3)
- FM Weirdness (Preset): [fm_weirdness.mp3](https://github.com/sjoerdvankreel/firefly-synth-storage/raw/main/render/fm_weirdness.mp3)
- AM Bells (Preset): [am_bells_reaper_clap.mp3](https://github.com/sjoerdvankreel/firefly-synth-storage/raw/main/render/am_bells_reaper_clap.mp3)
- AM/FM Unison Pad (Preset): [am_fm_unison_pad.mp3](https://github.com/sjoerdvankreel/firefly-synth-storage/raw/main/render/am_fm_unison_pad.mp3)
- Karplus-Strong (Preset): [karplus_strong_reaper_clap.mp3](https://github.com/sjoerdvankreel/firefly-synth-storage/raw/main/render/karplus_strong_reaper_clap.mp3)
- Hardcore kicks take 2 (Preset): [hardcore_kicks_take_2.mp3](https://github.com/sjoerdvankreel/firefly-synth-storage/raw/main/render/hardcore_kicks_take_2.mp3)
- I Love Distortion (Preset): [i_love_distortion_reaper_clap.mp3](https://github.com/sjoerdvankreel/firefly-synth-storage/raw/main/render/i_love_distortion_reaper_clap.mp3)
- Fun With Hard Sync And FM (Preset): [fun_with_hard_sync_and_fm.mp3](https://github.com/sjoerdvankreel/firefly-synth-storage/raw/main/render/fun_with_hard_sync_and_fm.mp3)
- Another acid line bipolar FM (Preset): [another_acid_line_bipolar_fm.mp3](https://github.com/sjoerdvankreel/firefly-synth-storage/raw/main/render/another_acid_line_bipolar_fm.mp3)
- Another acid line unipolar FM (Preset): [another_acid_line_unipolar_fm.mp3](https://github.com/sjoerdvankreel/firefly-synth-storage/raw/main/render/another_acid_line_unipolar_fm.mp3)
- Another acid line backward FM (Preset): [another_acid_line_backward_fm.mp3](https://github.com/sjoerdvankreel/firefly-synth-storage/raw/main/render/another_acid_line_backward_fm.mp3)

# Build from source
- git clone --recursive [this-project]
- Mac: scripts/build_mac.sh [Debug|RelWithDebInfo|Release]
- Linux: scripts/build_linux.sh [Debug|RelWithDebInfo|Release]
- Windows: scripts/build_windows.bat [Debug|RelWithDebInfo|Release]

Note: the build scripts and cmake files assume linux=gcc, windows=msvc and mac=clang.
It is not possible to change compiler for any given OS without changing the build scripts and cmake files.

You'll need CMake and a very recent c++ compiler. The final output ends up in the /dist folder. <br/>
Like noted above, the default Linux build does *not* enable AVX.
To fix that, you'll have to edit [https://github.com/sjoerdvankreel/firefly-synth/blob/main/plugin_base/cmake/plugin_base.config.cmake](https://github.com/sjoerdvankreel/firefly-synth/blob/main/plugin_base/cmake/plugin_base.config.cmake).

Windows: the build scripts assume 7zip is installed. It will build fine without, but you'll error out on the final zip step.

# Dependencies
- CLAP SDK: [https://github.com/free-audio/clap](https://github.com/free-audio/clap)
- JUCE: [https://github.com/juce-framework/JUCE](https://github.com/juce-framework/JUCE)
- MTS-ESP: [https://github.com/ODDSound/MTS-ESP](https://github.com/ODDSound/MTS-ESP)
- sse2neon: [https://github.com/DLTcollab/sse2neon](https://github.com/DLTcollab/sse2neon)
- Steinberg VST3 SDK: [https://github.com/steinbergmedia/vst3sdk](https://github.com/steinbergmedia/vst3sdk)
- Readerwriterqueue: [https://github.com/cameron314/readerwriterqueue](https://github.com/cameron314/readerwriterqueue)

# Special thanks to
- [ODDSound](https://oddsound.com/) for providing an MTS-ESP NFR.
- [BaconPaul](https://baconpaul.org/) for helping out with CLAP support.
- [oddyolynx](https://github.com/tank-trax) for supporting the project early-on.
- Host vendors who have set me up with an NFR license.
- Everyone from [Surge Synth Team](https://surge-synth-team.org/) microtuning discord channel for helping out with MTS-ESP support.

# Credits
- Contains soft clippers by [Sean Enderby and Zlatko Baracskai](https://dafx.de/paper-archive/2012/papers/dafx12_submission_45.pdf).
- Contains a slightly adapted implementation of [Jezar's Freeverb](https://github.com/sinshu/freeverb).
- Contains a slightly adapted implementation of the [Karplus-Strong algorithm](https://blog.demofox.org/2016/06/16/synthesizing-a-pluked-string-sound-with-the-karplus-strong-algorithm).
- Contains a verbatim implementation of [Andrew Simper's state variable filter equations](https://cytomic.com/files/dsp/SvfLinearTrapOptimised2.pdf).
- Contains a verbatim implementation of [Moorer's DSF algorithm as described by Burkhard Reike](https://www.verklagekasper.de/synths/dsfsynthesis/dsfsynthesis.html).
