### April 29, 2024 - V1.7.4 (No binaries).

- Added waveform 13 to the list of supported hosts. No idea if some update between waveform 12 and 13 fixed it, or there was a bug on firefly's side that got fixed.
- Unfortunately still no renoise and cakewalk.

### April 28, 2024 - V1.7.3.

- Swapped envelope sustain and smoothing in the gui.
- Bugfix: Shrunk FX title size to fit on mac.
- Bugfix: Do not store plugin settings directly under linux user home folder.
- Bugfix: internal block splitting did not respect sample accurate automation.
- Bugfix: automation changes were reported back to the host, which caused automation to stop working entirely in bitwig and flstudio.

### April 22, 2024 - V1.7.2.

- Updated dependencies to latest (juce, vst3, clap).
- Bugfix: the windows .zip archives are now actually .zip archives, not zipped .tar files.
- Breaking change: reduced max oversampling to 4x to cut down on memory usage.
- Breaking change: reduced max voice count from 128 to 64 (more memory cutdown). Global unison voices also come out of the 64 available!
- Split up internal processing into multiple fixed size buffers to not have memory usage dependent on the host buffer size.
-- This costs a bit of cpu, hopefully not too bad.

### April 13, 2024 - V1.7.1.

- Changed built-in themes (and dropped some).

### April 10, 2024 - V1.7.0.

- Rearranged the gui to have elements more aligned.
- Changed default distortion mode to plain tanh clipper.
- Renamed distortion skew in/out to x/y to align with lfo.
- Add additional global aux parameter just to fit the gui.
- Bugfix: disable envelope trigger for polyphonic mode.
- Bugfix: disable global unison parameters when voice count is 1.
- Bugfix + breaking change: when global unison is enabled voices are attenuated by sqrt(voice-count).

### April 2, 2024 - V1.6.1.

- Add Intel Mac support.

### March 31, 2024 - V1.6.0.

- Add Mac support. Apple silicon only, no Intel Macs yet!

### March 28, 2024 - V1.5.1 (No binaries).

- Add new preset (big pad 3 with global unison).

### March 27, 2024 - V1.5.0.

- Add global unison support. 
  Heavy on the cpu, benefits from CLAP threadpool.

### March 24, 2024 - V1.4.1.

- Add some color schemes.
- Add version info to gui.
- Add system DPI awareness.
- Increase undo history size.
- Organize presets in folders.
- Copy/paste patch from system clipboard.

### March 18, 2024 - V1.4.0.

- Add some soft clippers.

### March 15, 2024 - V1.3.1.

- Add some themes (just basic color schemes).

### March 14, 2024 - V1.3.0.

- Add theming support.
- Add 4 themes: current, current flat, infernal, infernal flat (re-introduced everyone's favourite creepy images!)
- Added a new preset.

### March 10, 2024 - V1.2.4 (No binaries).

- Swap out native message boxes for juce alert windows so they react to content scaling.

### March 10, 2024 - V1.2.3 (No binaries).

- Bugfix: undo menu did not react to content scaling.

### March 9, 2024 - V1.2.2 (No binaries).

- Change max ui scale factor from 200% to 800%.

### March 8, 2024 - V1.2.1.

- Add big pad 2 preset.
- Scale back default ui width to 1280.
- Bugfix: osc graph scales to 100% * gain.
- Bugfix: default global SVF keyboard tracking to 0.
- UI scaling from 50% to 200% (was 100 to unlimited).

### March 6, 2024 - V1.2.0.

- Lots of small ui fixes / cleanup.
- Rename saw waveshaper to off.
- Add 2 extra master cv aux parameters.
- Fixed a bug in the triangle waveshaper.
- Drop square waveshaper (bug, it shouldn't have been there).
- Comb filter gets additional mode select (feedback/feedforward/both).
- Split out combined comboboxes to separate controls:
  - NOTE! This is a breaking change for automation on those parameters.
  - LFO mode + sync.
  - Delay mode + sync.
  - LFO shape + skew controls.
  - FX Type + distortion mode.
  - Envelope type + slope mode.
  - Distortion shaper + skew controls.
- Rearranged lfo/distortion shape options
  - NOTE! This is a breaking change for automation on this parameter.

Even though there are some breaking changes I decided *against* changing the plugin id,
because breaking changes relate to automation of discrete-valued parameters only.
Regular loading of projects/preset files applies the appropriate conversion from previous formats.

### February 25, 2024 - V1.11.

- Add some more presets and fix some existing ones.

### February 25, 2024 - V1.1.

- Allow envelopes as target in CV-to-CV matrix. Envelopes can be modulated by master in section or global lfos.

### February 19, 2024 - V1.09.

- Add fx-only plugin version.

### February 14, 2024 - V1.08.

- Bugfix: reset voice manager state upon plugin (de)activate.
- Bugfix: do not assume CLAP host can do threadpool.
- Bugfix: do not assume CLAP host can do ostream with unlimited size.
- Bugfix: do not report stale value from CLAP gui to host, sync with audio first.

### February 12, 2024 - V1.07 (No binaries).

- Add another preset (matching to infernal synth's acid line).

### February 12, 2024 - V1.06.

- Add monophonic mode.
- Add another preset to go with it.

### February 10, 2024 - V1.05.

- Add some presets.
- Add an extra oscillator.
- LFO x/y become continuous parameters.
- Bugfix: Basic oscillator with no waveforms must not be processed.
- Add CV-to-CV routing matrices (modulate LFO by another LFO or envelope).

### February 8, 2024 - V1.04 (No binaries).

- Add scale and offset for CV matrices.

### February 3, 2024 - V1.03 (No binaries).

- Add manual / overview document.

### February 2, 2024 - V1.02 (No binaries).

- Add detailed parameter reference.

### January 31, 2024 - V1.01 (No binaries).

- Add per-oscillator gain control.
- Make the ui a bit wider to prevent gui clipping.
- Bugfix: do not limit whitenoise oscillator to nyquist.
- Bugfix: envelope whould incorrectly show as tempo-synced.

### January 27, 2024 - V1.0.

- Initial release.