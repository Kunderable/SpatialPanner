# Changelog

All notable changes to Spatial Panner. This file doubles as a development log so the
intent behind each feature is preserved.

## v1.0 — Initial release

The plugin grew iteratively from a single-track panner into a full visual spatial mixer.

### Audio engine
- Equal-power pan from `posX` (−1…+1).
- Depth from `posY` (0…1) as **pure gain attenuation** (0 → −15 dB). Reverb was
  intentionally **removed** — "further away" means quieter, not wetter.
- Mid/Side stereo **width** (`width`, 0…2).
- Per-track L/R peak meters with ballistic decay.
- Real-time stereo **correlation** metering.
- **Environment presets** (Studio / Club / Car / Phone) using `juce::Reverb` +
  `juce::IIRFilter` EQ bands, to audition how a mix translates to real systems.
- Lock-free **scope ring buffer** (2048 samples) feeding the vectorscope.

### Cross-instance design
- `GlobalSpatialRegistry` singleton lets every plugin instance see and control every
  other track from one canvas (no per-track window juggling). VST3 cannot read other
  tracks, so each track carries its own instance and they rendezvous in-process.
- `alive` flag + message-thread marshalling so editors never touch a destroyed
  processor.

### UI / UX
- 2D **stage canvas** with listener at bottom-centre, full perspective depth arcs with
  −dB labels, breathing background glow.
- Track **markers**: rotating diamond + ring (replaced the plain sphere), colour per
  track, label pill.
- Glowing **width ovals** with arrow end-caps; pulse with level.
- **Track list** sidebar with click-to-select/lock (non-selected tracks dim).
- **Auto track names** from the host (`updateTrackProperties`), manually editable.
- **dB meter** redesigned as LED segments (green→amber→red) with falling peak and a
  **latched peak readout** that resets on click.
- **Correlation** bar.
- **Vectorscope** (SCOPE button): polar/dome CRT scope with **phosphor persistence**
  (fading trails), additive-style bright cores, neon dome.
- **WIDTH** slider moved to a large bottom strip; controls the **selected** track and
  syncs across instances.
- **Reference layouts** (REF button → animated **genre picker** overlay):
  ghost markers showing pro-standard position **and suggested width** per instrument.
  Genres: House, Deep House, Progressive House, Melodic House, Tech House, Techno,
  Trance, Future Bass, Dubstep, Drum & Bass, Trap, Pop, Rock, Metal, Hip-Hop, R&B,
  Jazz, Acoustic, Orchestral.
- **Resizable** window with proportional UI scaling.
- S1-Imager-inspired grey-metal chrome with animated title (sweeping highlight,
  pulsing accent, rotating logo diamond).
- State (pan/depth/width/name/environment) persisted with the project.

### Notes for future work
- A separate **Max for Live** prototype (`../SpatialMixer/`) was explored to read real
  Ableton track names/levels and control all tracks from one master device. The VST3
  approach (one instance per track + shared registry) was chosen as the main product
  because it is host-agnostic.
