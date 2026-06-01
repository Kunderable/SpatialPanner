# Spatial Panner

A VST3/AU plugin for mixing by placing tracks on a 2D stage instead of fiddling with
separate pan and volume faders. Each track is a dot: move it left/right to pan, move it
up/down to push it forward or back (back = quieter). Every instance of the plugin shares
one canvas, so from any track you can see and drag every other track.

Built with JUCE 8 and CMake.

## How it works

You put an instance on each track you want to position. The horizontal axis is
equal-power pan. The vertical axis is depth, applied as plain gain (front = 0 dB,
back = up to -15 dB). There is no reverb on depth on purpose: moving a sound back just
makes it more distant/quieter. There's also a per-track stereo width control (mid/side),
drawn as an oval around the dot.

All instances talk to each other through a single in-process registry, so the canvas on
any track shows the whole mix and lets you move any dot.

## Features

- One dot per track on a shared canvas, drag from anywhere.
- Track list sidebar; click a track to lock it so only that one moves.
- Track names picked up automatically from the host, editable by hand.
- Per-track L/R meters with falling peak and a latched peak you can click to reset.
- Stereo correlation meter for the selected track.
- Vectorscope view (polar scope with phosphor-style trails).
- Width slider that drives the selected track and stays in sync across instances.
- Listening-environment presets (Studio/Club/Car/Phone) using reverb + EQ, to check how
  a mix translates.
- Genre reference layouts shown as ghost markers with suggested position and width.
- Scroll-wheel zoom + pan for precise placement.
- Resizable window.
- Pan/depth/width/name/environment saved with the project.

## Build

Needs CMake 3.22+ and a C++17 compiler. JUCE is fetched automatically.

```bash
cd SpatialPanner
bash build.sh
```

Output goes to `build/SpatialPanner_artefacts/Release/`.

Install on macOS:

```bash
cp -r "build/SpatialPanner_artefacts/Release/VST3/Spatial Panner.vst3"    ~/Library/Audio/Plug-Ins/VST3/
cp -r "build/SpatialPanner_artefacts/Release/AU/Spatial Panner.component"  ~/Library/Audio/Plug-Ins/Components/
```

Then rescan plugins in your DAW.

## Code layout

- `Source/PluginProcessor.*` - audio: equal-power pan, depth gain, M/S width, meters,
  correlation, environment reverb/EQ, scope ring buffer. APVTS params: posX, posY, width.
- `Source/GlobalSpatialRegistry.*` - process-wide singleton every instance registers
  with, so all editors see the same tracks. Stores raw param/meter/scope pointers behind
  an alive flag.
- `Source/MultiCanvas.*` - the 2D stage: tracks, crosshairs, width ovals, depth rings,
  vectorscope, reference overlay, zoom/pan, drag handling.
- `Source/GenreData.h` - reference layouts per genre (positions + suggested widths).
- `Source/PluginEditor.*` - the UI: title bar, track list, meter panel, correlation bar,
  width slider, environment buttons, genre picker, window scaling.

Because all instances live in the host process there's no IPC; the singleton registry is
enough. Each processor clears its alive flag in the destructor before deregistering so
editors never touch a dead instance.
