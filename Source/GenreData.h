#pragma once
#include <JuceHeader.h>
#include <vector>
#include "MultiCanvas.h"

// Genre reference layouts based on common professional mixing practice.
// x: -1..1 (L..R), y: 0..1 (front..back). Depths spread for readability.
namespace GenreData
{
    struct GenreDef { const char* category; const char* name; };

    // Master list - grouped by category, shown in the picker.
    inline const std::vector<GenreDef>& list()
    {
        static const std::vector<GenreDef> g = {
            { "ELECTRONIC", "House" },
            { "ELECTRONIC", "Deep House" },
            { "ELECTRONIC", "Progressive House" },
            { "ELECTRONIC", "Melodic House" },
            { "ELECTRONIC", "Tech House" },
            { "ELECTRONIC", "Techno" },
            { "ELECTRONIC", "Trance" },
            { "ELECTRONIC", "Future Bass" },
            { "ELECTRONIC", "Dubstep" },
            { "ELECTRONIC", "Drum & Bass" },
            { "ELECTRONIC", "Trap" },
            { "BAND / SONG", "Pop" },
            { "BAND / SONG", "Rock" },
            { "BAND / SONG", "Metal" },
            { "BAND / SONG", "Hip-Hop" },
            { "BAND / SONG", "R&B" },
            { "BAND / SONG", "Jazz" },
            { "BAND / SONG", "Acoustic" },
            { "BAND / SONG", "Orchestral" },
            { "GRAMMY ELECTRONIC", "Daft Punk" },
            { "GRAMMY ELECTRONIC", "Skrillex" },
            { "GRAMMY ELECTRONIC", "The Chemical Brothers" },
            { "GRAMMY ELECTRONIC", "Flume" },
            { "GRAMMY ELECTRONIC", "Kaytranada" },
            { "GRAMMY ELECTRONIC", "Rufus Du Sol" },
            { "GRAMMY ELECTRONIC", "Zedd" },
            { "GRAMMY ELECTRONIC", "Justice" },
            { "GRAMMY ELECTRONIC", "Fred again.." },
            { "GRAMMY ELECTRONIC", "Jack U" },
        };
        return g;
    }

    using RP = MultiCanvas::RefPoint;

    inline std::vector<RP> forGenre(const juce::String& n)
    {
        // House family
        if (n == "House" || n == "Tech House")
            return {
                {"Kick",  0.00f,0.04f}, {"Sub",   0.00f,0.09f}, {"Bass", 0.00f,0.14f},
                {"Clap",  0.00f,0.20f}, {"Snare", 0.00f,0.24f},
                {"Hat",   0.28f,0.18f}, {"OpenHat",-0.28f,0.22f},
                {"Perc L",-0.45f,0.30f},{"Perc R", 0.45f,0.32f},
                {"Stab L",-0.58f,0.40f},{"Stab R", 0.58f,0.42f},
                {"Vocal", 0.00f,0.12f}, {"Lead",   0.00f,0.34f},
                {"Pad L",-0.72f,0.55f}, {"Pad R",  0.72f,0.56f},
                {"FX L", -0.85f,0.66f}, {"FX R",   0.85f,0.66f},
            };
        if (n == "Deep House")
            return {
                {"Kick",  0.00f,0.05f}, {"Bass", 0.00f,0.13f},
                {"Rim",   0.10f,0.20f}, {"Hat",  0.30f,0.18f}, {"Shaker",-0.30f,0.22f},
                {"Chord L",-0.55f,0.38f},{"Chord R",0.55f,0.40f},
                {"Rhodes",-0.35f,0.34f},{"Vocal",0.00f,0.14f},
                {"Pad L",-0.78f,0.58f}, {"Pad R", 0.78f,0.60f},
                {"Atmos L",-0.9f,0.70f},{"Atmos R",0.9f,0.72f},
            };
        if (n == "Progressive House" || n == "Melodic House")
            return {
                {"Kick",  0.00f,0.05f}, {"Sub",  0.00f,0.10f}, {"Bass", 0.00f,0.16f},
                {"Clap",  0.00f,0.22f}, {"Hat",  0.26f,0.20f}, {"Ride",-0.26f,0.24f},
                {"Pluck L",-0.50f,0.36f},{"Pluck R",0.50f,0.38f},
                {"Lead",  0.00f,0.40f}, {"Vocal",0.00f,0.13f},
                {"Arp L",-0.62f,0.46f}, {"Arp R", 0.62f,0.48f},
                {"Pad L",-0.82f,0.62f}, {"Pad R", 0.82f,0.64f},
                {"Atmos",0.00f,0.78f},
            };
        if (n == "Trance")
            return {
                {"Kick",  0.00f,0.05f}, {"Bass", 0.00f,0.14f}, {"Offbeat",0.00f,0.18f},
                {"Clap",  0.00f,0.22f}, {"Hat",  0.30f,0.20f}, {"OpenHat",-0.30f,0.24f},
                {"Supersaw L",-0.78f,0.42f},{"Supersaw R",0.78f,0.42f},
                {"Lead",  0.00f,0.40f}, {"Pluck L",-0.50f,0.36f},{"Pluck R",0.50f,0.38f},
                {"Pad L",-0.88f,0.60f}, {"Pad R", 0.88f,0.62f}, {"FX",0.00f,0.80f},
            };
        if (n == "Techno")
            return {
                {"Kick",  0.00f,0.04f}, {"Bass", 0.00f,0.12f}, {"Rumble",0.00f,0.18f},
                {"Clap",  0.05f,0.22f}, {"Hat",  0.32f,0.20f}, {"Perc L",-0.40f,0.28f},
                {"Perc R", 0.40f,0.30f},{"Stab L",-0.55f,0.40f},{"Stab R",0.55f,0.42f},
                {"Atmos L",-0.80f,0.60f},{"Atmos R",0.80f,0.62f},{"FX",0.00f,0.78f},
            };
        if (n == "Future Bass")
            return {
                {"Kick",  0.00f,0.05f}, {"808",  0.00f,0.10f},
                {"Snare", 0.00f,0.22f}, {"Hat",  0.28f,0.18f},
                {"Chord L",-0.80f,0.40f},{"Chord R",0.80f,0.40f},
                {"Lead",  0.00f,0.36f}, {"Vocal",0.00f,0.12f}, {"Vox Chop L",-0.5f,0.3f},
                {"Vox Chop R",0.5f,0.32f},{"Pad L",-0.9f,0.62f},{"Pad R",0.9f,0.64f},
            };
        if (n == "Dubstep")
            return {
                {"Kick",  0.00f,0.05f}, {"Sub",  0.00f,0.10f}, {"Reese",0.00f,0.30f},
                {"Snare", 0.00f,0.22f}, {"Hat",  0.30f,0.18f},
                {"Growl L",-0.45f,0.34f},{"Growl R",0.45f,0.36f},
                {"Lead",  0.00f,0.40f}, {"FX L",-0.85f,0.6f},{"FX R",0.85f,0.62f},
            };
        if (n == "Drum & Bass")
            return {
                {"Kick",  0.00f,0.05f}, {"Sub",  0.00f,0.10f}, {"Reese",0.00f,0.28f},
                {"Snare", 0.00f,0.20f}, {"Break L",-0.35f,0.26f},{"Break R",0.35f,0.28f},
                {"Hat",   0.30f,0.18f}, {"Pad L",-0.80f,0.55f},{"Pad R",0.80f,0.57f},
                {"Atmos",0.00f,0.75f},
            };
        if (n == "Trap")
            return {
                {"808",   0.00f,0.04f}, {"Kick", 0.00f,0.08f}, {"Snare",0.00f,0.20f},
                {"Hat",   0.18f,0.14f}, {"OpenHat",-0.18f,0.18f}, {"Perc",-0.35f,0.26f},
                {"Vocal", 0.00f,0.06f}, {"Ad-lib L",-0.75f,0.30f},{"Ad-lib R",0.75f,0.32f},
                {"Melody",0.00f,0.34f}, {"FX L",-0.85f,0.55f},{"FX R",0.85f,0.57f},
            };

        // Band / song
        if (n == "Pop")
            return {
                {"Kick",  0.00f,0.05f}, {"Bass", 0.00f,0.12f}, {"Snare",0.00f,0.16f},
                {"Hat",   0.25f,0.18f}, {"Clap", 0.00f,0.20f},
                {"Gtr L",-0.50f,0.28f}, {"Gtr R", 0.50f,0.30f},
                {"Keys",  0.30f,0.34f}, {"Vocal",0.00f,0.02f}, {"BGV L",-0.55f,0.24f},
                {"BGV R", 0.55f,0.26f}, {"Pad L",-0.78f,0.55f},{"Pad R",0.78f,0.57f},
            };
        if (n == "Rock" || n == "Metal")
            return {
                {"Kick",  0.00f,0.05f}, {"Bass", 0.00f,0.12f}, {"Snare",0.00f,0.14f},
                {"Hat",   0.25f,0.18f}, {"Tom L",-0.40f,0.22f},{"Tom R",0.40f,0.24f},
                {"OH L", -0.70f,0.30f}, {"OH R",  0.70f,0.32f},
                {"Gtr L",-0.65f,0.20f}, {"Gtr R", 0.65f,0.22f},
                {"Lead Gtr",0.30f,0.36f},{"Vocal",0.00f,0.02f}, {"Keys",-0.35f,0.40f},
            };
        if (n == "Hip-Hop")
            return {
                {"808",   0.00f,0.04f}, {"Kick", 0.00f,0.08f}, {"Snare",0.00f,0.16f},
                {"Hat",   0.20f,0.14f}, {"Perc",-0.30f,0.24f}, {"Sample L",-0.45f,0.34f},
                {"Sample R",0.45f,0.36f},{"Vocal",0.00f,0.02f},{"Ad-lib L",-0.72f,0.28f},
                {"Ad-lib R",0.72f,0.30f},{"Keys",0.35f,0.40f},
            };
        if (n == "R&B")
            return {
                {"Kick",  0.00f,0.06f}, {"Bass", 0.00f,0.13f}, {"Snare",0.00f,0.18f},
                {"Hat",   0.22f,0.16f}, {"Rhodes L",-0.45f,0.32f},{"Rhodes R",0.45f,0.34f},
                {"Gtr",  -0.35f,0.28f}, {"Vocal",0.00f,0.02f}, {"BGV L",-0.6f,0.22f},
                {"BGV R", 0.6f,0.24f},  {"Pad L",-0.8f,0.55f}, {"Pad R",0.8f,0.57f},
            };
        if (n == "Jazz")
            return {
                {"Kick",  0.00f,0.10f}, {"Upright",-0.20f,0.16f}, {"Snare",0.10f,0.18f},
                {"Ride",  0.35f,0.24f}, {"Piano L",-0.55f,0.30f},{"Piano R",-0.25f,0.34f},
                {"Sax",   0.30f,0.26f}, {"Trumpet",-0.30f,0.28f},{"Vocal",0.00f,0.06f},
            };
        if (n == "Acoustic")
            return {
                {"Gtr",  -0.25f,0.18f}, {"Bass", 0.00f,0.14f}, {"Cajon",0.10f,0.20f},
                {"Shaker",0.35f,0.24f}, {"Vocal",0.00f,0.04f}, {"Harmony L",-0.5f,0.20f},
                {"Harmony R",0.5f,0.22f},{"Strings L",-0.75f,0.5f},{"Strings R",0.75f,0.52f},
            };
        if (n == "Orchestral")
            return {
                {"Timpani",0.10f,0.55f},{"Bass",-0.30f,0.45f}, {"Cello",-0.45f,0.38f},
                {"Viola", -0.20f,0.34f},{"Violin 1",-0.55f,0.28f},{"Violin 2",-0.30f,0.30f},
                {"Brass",  0.35f,0.48f},{"Horns", 0.20f,0.52f}, {"Woodwind",0.00f,0.40f},
                {"Flute",  0.30f,0.36f},{"Harp",-0.40f,0.42f},  {"Perc R",0.55f,0.50f},
            };

        // Grammy-winning electronic acts (Best Dance/Electronic). Signatures are
        // approximate of their production style; educational references.
        if (n == "Daft Punk")          // French house, vocoder lead, wide funk (Random Access Memories)
            return {
                {"Kick",  0.00f,0.05f,0.4f}, {"Bass", 0.00f,0.11f,0.5f}, {"Clap",0.00f,0.18f,0.8f},
                {"Hat",   0.25f,0.16f,0.7f}, {"Vocoder",0.00f,0.10f,0.9f},
                {"Gtr L",-0.60f,0.28f,1.3f}, {"Gtr R", 0.60f,0.30f,1.3f},
                {"Synth L",-0.80f,0.36f,1.7f},{"Synth R",0.80f,0.36f,1.7f},
                {"Strings L",-0.95f,0.55f,2.0f},{"Strings R",0.95f,0.55f,2.0f},
            };
        if (n == "Skrillex")           // dubstep/bass, mono lows, screaming wide mids
            return {
                {"Sub",  0.00f,0.06f,0.3f}, {"Kick", 0.00f,0.04f,0.3f}, {"Snare",0.00f,0.20f,0.7f},
                {"Hat",  0.20f,0.14f,0.7f}, {"Growl L",-0.55f,0.32f,1.6f},{"Growl R",0.55f,0.34f,1.6f},
                {"Lead", 0.00f,0.40f,1.0f}, {"Vox Chop L",-0.75f,0.22f,1.7f},{"Vox Chop R",0.75f,0.22f,1.7f},
                {"FX L",-0.95f,0.55f,2.0f}, {"FX R",0.95f,0.55f,2.0f},
            };
        if (n == "The Chemical Brothers") // big beat, breakbeats, psychedelic wide
            return {
                {"Kick",  0.00f,0.05f,0.4f}, {"Bass", 0.00f,0.12f,0.6f},
                {"Break L",-0.40f,0.22f,1.2f},{"Break R",0.40f,0.24f,1.2f},
                {"Acid", 0.00f,0.30f,1.0f}, {"Stab L",-0.65f,0.34f,1.5f},{"Stab R",0.65f,0.36f,1.5f},
                {"Vox", 0.00f,0.16f,0.8f}, {"Noise L",-0.90f,0.50f,1.9f},{"Noise R",0.90f,0.52f,1.9f},
            };
        if (n == "Flume")              // future bass, detuned wide chords, vocal chops
            return {
                {"Kick",  0.00f,0.05f,0.4f}, {"808", 0.00f,0.10f,0.4f}, {"Snare",0.00f,0.22f,0.7f},
                {"Hat",   0.28f,0.16f,0.7f}, {"Chord L",-0.85f,0.36f,1.9f},{"Chord R",0.85f,0.36f,1.9f},
                {"Lead", 0.00f,0.32f,1.0f}, {"Vox Chop L",-0.55f,0.24f,1.5f},{"Vox Chop R",0.55f,0.26f,1.5f},
                {"Texture L",-0.95f,0.6f,2.0f},{"Texture R",0.95f,0.6f,2.0f},
            };
        if (n == "Kaytranada")         // house/soul groove, warm, mid-wide
            return {
                {"Kick",  0.00f,0.05f,0.4f}, {"Bass", 0.00f,0.12f,0.5f}, {"Clap",0.00f,0.18f,0.8f},
                {"Hat",   0.22f,0.15f,0.7f}, {"Perc",-0.30f,0.22f,1.0f},
                {"Rhodes L",-0.50f,0.30f,1.3f},{"Rhodes R",0.50f,0.32f,1.3f},
                {"Vocal", 0.00f,0.12f,0.7f}, {"Synth L",-0.72f,0.40f,1.6f},{"Synth R",0.72f,0.42f,1.6f},
            };
        if (n == "Rufus Du Sol")       // melodic/organic house, lush wide pads
            return {
                {"Kick",  0.00f,0.05f,0.4f}, {"Sub",  0.00f,0.10f,0.3f}, {"Bass",0.00f,0.15f,0.5f},
                {"Clap",  0.00f,0.20f,0.8f}, {"Hat",  0.26f,0.18f,0.7f},
                {"Vocal", 0.00f,0.12f,0.7f}, {"Pluck L",-0.55f,0.34f,1.3f},{"Pluck R",0.55f,0.36f,1.3f},
                {"Pad L",-0.90f,0.55f,1.9f}, {"Pad R", 0.90f,0.58f,1.9f}, {"Atmos",0.00f,0.78f,1.6f},
            };
        if (n == "Zedd")               // electro house, bright supersaw, festival wide
            return {
                {"Kick",  0.00f,0.05f,0.4f}, {"Bass", 0.00f,0.14f,0.5f}, {"Clap",0.00f,0.20f,0.8f},
                {"Hat",   0.30f,0.18f,0.7f}, {"Vocal",0.00f,0.10f,0.7f},
                {"Supersaw L",-0.85f,0.36f,1.9f},{"Supersaw R",0.85f,0.36f,1.9f},
                {"Pluck L",-0.55f,0.30f,1.3f},{"Pluck R",0.55f,0.32f,1.3f},
                {"FX L",-0.95f,0.55f,2.0f}, {"FX R",0.95f,0.55f,2.0f},
            };
        if (n == "Justice")            // French electro, distorted bass, gritty wide
            return {
                {"Kick",  0.00f,0.05f,0.4f}, {"Bass", 0.00f,0.12f,0.7f}, {"Clap",0.05f,0.20f,0.8f},
                {"Hat",   0.28f,0.16f,0.7f}, {"Stab L",-0.65f,0.30f,1.5f},{"Stab R",0.65f,0.32f,1.5f},
                {"Vocoder",0.00f,0.16f,0.9f}, {"Synth L",-0.85f,0.40f,1.8f},{"Synth R",0.85f,0.42f,1.8f},
                {"Noise",0.00f,0.6f,1.6f},
            };
        if (n == "Fred again..")       // UK electronic, intimate vox samples, textured
            return {
                {"Kick",  0.00f,0.05f,0.4f}, {"Sub",  0.00f,0.10f,0.3f}, {"Snare",0.00f,0.20f,0.7f},
                {"Hat",  -0.20f,0.16f,0.7f}, {"Vox Sample",0.00f,0.12f,0.9f},
                {"Chop L",-0.55f,0.26f,1.4f},{"Chop R",0.55f,0.28f,1.4f},
                {"Piano",-0.30f,0.34f,1.1f}, {"Pad L",-0.88f,0.55f,1.8f},{"Pad R",0.88f,0.55f,1.8f},
            };
        if (n == "Jack U")             // Skrillex x Diplo, trap/dance, hard-panned chaos
            return {
                {"808",  0.00f,0.05f,0.3f}, {"Kick", 0.00f,0.08f,0.3f}, {"Snare",0.00f,0.20f,0.7f},
                {"Hat",  0.18f,0.13f,0.7f}, {"Vocal",0.00f,0.10f,0.7f},
                {"Lead L",-0.70f,0.32f,1.6f},{"Lead R",0.70f,0.34f,1.6f},
                {"Vox Chop L",-0.95f,0.20f,1.9f},{"Vox Chop R",0.95f,0.22f,1.9f},
                {"FX",0.00f,0.55f,1.7f},
            };

        return {};
    }
}
