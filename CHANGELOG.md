# Changelog

## v1.0

First working version. Started as a single-track panner and grew into a shared
multi-track spatial mixer.

Audio:
- Equal-power pan from posX.
- Depth from posY as plain gain (0 to -15 dB), no reverb.
- Mid/side stereo width.
- Per-track L/R peak meters with ballistic decay.
- Stereo correlation metering.
- Environment presets (Studio/Club/Car/Phone) using juce::Reverb + IIR EQ to audition
  how a mix translates to different systems.
- Lock-free 2048-sample ring buffer feeding the vectorscope.

Cross-instance:
- GlobalSpatialRegistry singleton so every instance sees and can move every track from
  one canvas. VST3 can't read other tracks, so each track gets its own instance and they
  meet in-process.
- Alive flag + message-thread marshalling so editors never touch a destroyed processor.

UI:
- 2D stage with the listener at the bottom centre and concentric depth rings.
- Track markers (diamond + ring), colour per track, name pill.
- Width ovals with arrow caps that react to level.
- Track list sidebar with click-to-lock (other tracks dim).
- Auto track names from the host, editable.
- LED-segment dB meter with falling peak and a latched peak that resets on click.
- Correlation bar.
- Vectorscope (SCOPE button) with phosphor-style fading trails.
- Width slider drives the selected track and syncs across instances.
- Genre reference layouts (REF button opens a genre picker); ghost markers show
  suggested position and width.
- Scroll-wheel zoom and pan.
- Resizable window with proportional scaling.
- State saved with the project.

Notes:
- There's an earlier Max for Live prototype in ../SpatialMixer/ that reads real Ableton
  track names/levels from the Live API. The VST3 build was chosen as the main version
  because it works in any host.
