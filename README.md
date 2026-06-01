# Spatial Panner

A VST3 / AU plugin for **visual spatial mixing**. Place each track as a movable dot
on a 2D stage — horizontal position sets **pan**, vertical position sets **depth**
(distance attenuation, no reverb). Every instance of the plugin shares one global
canvas, so from any track you can see and move **all** tracks at once.

Built with [JUCE 8](https://juce.com/) and CMake.

![Spatial Panner](docs/screenshot.png)

---

## Why this exists

Standard mixers expose pan and volume as separate faders per track, which makes it
hard to *see* the stereo image as a whole. Spatial Panner turns mixing into a
spatial, visual task:

- One dot per track on a top-down "stage" (listener at the bottom-centre).
- **X axis** → equal-power pan (L ↔ R).
- **Y axis** → depth: front = full level, back = up to −15 dB quieter (pure gain,
  no reverb — the source just moves "further away").
- A **stereo-width** control per track (Mid/Side), shown as a glowing oval.
- All plugin instances see each other through a shared in-process registry, so the
  master (or any track) shows the entire mix layout.

---

## Features

| Area | What it does |
|------|--------------|
| **Multi-track canvas** | Every instance renders all registered tracks; drag any dot from anywhere. |
| **Track list** | Sidebar lists all tracks; click to select/lock so only that one moves (others dim). |
| **Auto track names** | Picks up the host track name via VST3 `updateTrackProperties`; editable manually. |
| **dB meters** | Per-track L/R LED-segment meters with falling peak + latched peak (click to reset). |
| **Correlation meter** | Real-time stereo correlation (−1…+1) of the selected track. |
| **Vectorscope** | Phosphor-persistence polar scope (CRT style) of the selected track's stereo field. |
| **Width sync** | The WIDTH slider drives the *selected* track and syncs across all instances. |
| **Environment presets** | Studio / Club / Car / Phone — real DSP (reverb + multiband EQ) to audition how a mix translates. |
| **Reference layouts** | Pro mixing-standard placements per genre (House, Techno, Trance, Rock, Hip-Hop, Orchestral, …) shown as ghost markers with suggested position **and** width. |
| **Resizable UI** | Drag the window corner; the whole UI scales proportionally. |
| **State persistence** | Pan / depth / width / name / environment saved with the project. |

---

## Build

Requires CMake ≥ 3.22 and a C++17 compiler. JUCE is fetched automatically.

```bash
cd SpatialPanner
bash build.sh          # configures + builds Release (VST3, AU, Standalone)
```

Artifacts land in `build/SpatialPanner_artefacts/Release/`.

### Install (macOS)

```bash
cp -r "build/SpatialPanner_artefacts/Release/VST3/Spatial Panner.vst3"   ~/Library/Audio/Plug-Ins/VST3/
cp -r "build/SpatialPanner_artefacts/Release/AU/Spatial Panner.component" ~/Library/Audio/Plug-Ins/Components/
```

Then rescan plugins in your DAW.

---

## Architecture

| File | Responsibility |
|------|----------------|
| `Source/PluginProcessor.{h,cpp}` | Audio: equal-power pan, depth gain, M/S width, meters, correlation, environment DSP (reverb + IIR EQ), scope ring buffer. APVTS params: `posX`, `posY`, `width`. |
| `Source/GlobalSpatialRegistry.{h,cpp}` | Process-wide singleton. Every instance registers its params + meter/scope pointers + an `alive` flag. Provides thread-safe snapshots and change listeners so all editors stay in sync. |
| `Source/MultiCanvas.{h,cpp}` | The 2D stage: draws all tracks, crosshairs, width ovals, perspective depth arcs, the vectorscope (phosphor buffer), and the reference ghost overlay. Handles drag/select. |
| `Source/GenreData.h` | Reference layout tables — per-genre instrument positions + suggested widths. |
| `Source/PluginEditor.{h,cpp}` | S1-Imager-style chrome: title bar, track list, right meter panel, correlation bar, WIDTH slider, environment buttons, animated genre picker (`GenrePanel`), UI scaling. |

### Cross-instance sync

There is no inter-process communication — all instances live in the host's process,
so a single `GlobalSpatialRegistry` (Meyers singleton) is enough. The registry stores
raw pointers to each processor's parameters and atomic meter/scope buffers, guarded by
an `alive` flag set false in the destructor before deregistration, so editors never
touch a dead processor.

### Depth model

`gain = dB→gain(−depth × 15 dB)`. Depth is **pure attenuation** (no reverb) — moving a
track "back" simply makes it quieter, as if further from the listener.

### Environment presets (audition modes)

These are global "listening environment" simulations applied after the spatial stage:
- **Studio** — light room, flat.
- **Club** — large reverb, +bass shelf, mid scoop, air shelf.
- **Car** — small boxy reverb, low/low-mid bumps, rolled-off highs.
- **Phone** — 300 Hz–3.4 kHz band-pass.

---

## License

Personal project. JUCE is used under its own license terms.
