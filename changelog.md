### ?, ? - V1.2.0.

- Lots of small ui fixes / cleanup.
- Comb filter gets additional mode select (feedback/feedforward/both).
- Split out distortion+mode selection to separate controls (breaking change for automation only).
- Split out distortion shape+skewx/y selection to separate controls (breaking change for automation only).

- Even though there are some breaking changes I decided *against* changing the plugin id,
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