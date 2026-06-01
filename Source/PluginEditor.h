#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "MultiCanvas.h"

// Track list sidebar
class TrackListPanel : public juce::Component
{
public:
    std::function<void(int id)> onSelect;  // -1 = deselect all

    void refresh()
    {
        tracks = GlobalSpatialRegistry::get().snapshot();
        repaint();
    }
    void setSelectedId(int id) { selectedId = id; repaint(); }

    void paint(juce::Graphics& g) override
    {
        const float w = getWidth(), h = getHeight();

        // Background
        g.setColour(juce::Colour(0xff080420));
        g.fillRect(getLocalBounds());

        // Header
        g.setFont(juce::Font(juce::FontOptions(8.f).withStyle("Bold")));
        g.setColour(juce::Colour(0xff5a4090));
        g.drawText("TRACKS", 0, 0, (int)w, 18, juce::Justification::centred);
        g.setColour(juce::Colour(0xff1a0e44));
        g.drawLine(4.f, 18.f, w - 4.f, 18.f, 0.5f);

        // Rows
        int y = 20;
        for (auto& t : tracks) {
            const bool sel = t.id == selectedId;
            if (sel) {
                g.setColour(t.colour.withAlpha(0.18f));
                g.fillRect(0, y, (int)w, kRowH);
                g.setColour(t.colour.withAlpha(0.6f));
                g.fillRect(0, y, 2, kRowH);
            }
            // Colour dot
            g.setColour(t.colour.withAlpha(sel ? 1.f : 0.6f));
            g.fillEllipse(6.f, y + (kRowH - 8) * 0.5f, 8.f, 8.f);
            // Label
            g.setFont(juce::Font(juce::FontOptions(9.5f).withStyle(sel ? "Bold" : "Regular")));
            g.setColour(sel ? juce::Colour(0xffddd6fe) : juce::Colour(0xff6a52a0));
            g.drawText(t.label, 18, y, (int)w - 22, kRowH, juce::Justification::centredLeft);

            y += kRowH;
        }

        // Border right
        g.setColour(juce::Colour(0xff1c1048));
        g.drawLine(w - 1.f, 0.f, w - 1.f, h, 1.f);
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        const int idx = (e.getPosition().y - 20) / kRowH;
        if (idx >= 0 && idx < (int)tracks.size()) {
            const int newId = tracks[idx].id;
            selectedId = (selectedId == newId) ? -1 : newId;
        } else {
            selectedId = -1;
        }
        if (onSelect) onSelect(selectedId);
        repaint();
    }

private:
    std::vector<TrackEntry> tracks;
    int selectedId = -1;
    static constexpr int kRowH = 26;
};

// Animated genre picker overlay
class GenrePanel : public juce::Component, private juce::Timer
{
public:
    std::function<void(juce::String)> onPick;   // "" = clear
    std::function<void()>             onClosed;

    GenrePanel();
    void showPanel();
    void hidePanel();
    bool isOpen() const { return open; }

    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent&) override;

private:
    struct Chip { juce::String cat, name; juce::Rectangle<int> bounds; };
    std::vector<Chip> chips;
    int   hoverIdx = -1;
    bool  open = false, closing = false;
    float anim = 0.f;   // 0..1

    juce::Rectangle<int> cardBounds;

    void timerCallback() override;
    void buildChips();
    void mouseMove(const juce::MouseEvent&) override;
    int  chipAt(juce::Point<int>) const;
};

// Main Editor
class SpatialPannerAudioProcessorEditor
    : public juce::AudioProcessorEditor,
      public juce::Timer
{
public:
    explicit SpatialPannerAudioProcessorEditor(SpatialPannerAudioProcessor&);
    ~SpatialPannerAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;
    void mouseDown(const juce::MouseEvent&) override;

private:
    SpatialPannerAudioProcessor& processorRef;

    TrackListPanel   trackList;
    MultiCanvas      canvas;
    juce::Slider     widthSlider;   // large, at bottom
    juce::TextEditor nameEditor;
    juce::Label      statusLabel;
    juce::TextButton scopeBtn;      // toggles vectorscope view
    juce::TextButton refBtn;        // opens genre picker
    GenrePanel       genrePanel;
    juce::String     refGenre;      // active reference genre ("" = none)
    bool             scopeOn = false;
    float            uiScale = 1.f;

    // Right meter state
    int   focusedTrackId = -1;
    float meterLDisplay  = 0.f, meterRDisplay = 0.f;
    float peakLDisplay   = 0.f, peakRDisplay  = 0.f;
    float peakLatchL     = 0.f, peakLatchR    = 0.f;  // latched maxima (manual reset)
    float corrDisplay    = 0.f;
    int   peakHoldFrames = 0;

    juce::Rectangle<int> peakResetBounds;  // click here to reset latched peaks
    float uiPhase = 0.f;                    // global animation phase

    // Environment preset buttons
    std::array<juce::TextButton, 4> envBtns;
    int activeEnvIdx = 0;  // 0=Off shown as "Studio" button state

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> widthAttach;

    void drawTitleBar(juce::Graphics& g);
    void drawRightMeter(juce::Graphics& g, juce::Rectangle<int> b);
    void drawCorrelation(juce::Graphics& g, juce::Rectangle<int> b);
    void updateEnvButtons();
    void applyReference(const juce::String& genre);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpatialPannerAudioProcessorEditor)
};
