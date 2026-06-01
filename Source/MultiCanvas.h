#pragma once
#include <JuceHeader.h>
#include "GlobalSpatialRegistry.h"

class MultiCanvas : public juce::Component, public juce::Timer
{
public:
    MultiCanvas();
    ~MultiCanvas() override;

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
    void mouseDoubleClick(const juce::MouseEvent&) override;
    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    void timerCallback() override;

    // Set which track is "locked" (only this one can be dragged). -1 = all.
    void setLockedTrackId(int id) { lockedToId = id; selectedId = id; repaint(); }
    int  getSelectedId() const { return selectedId; }

    // Vectorscope mode
    void setScopeMode(bool on) { scopeMode = on; repaint(); }
    void setFocusedId(int id)  { focusedId = id; repaint(); }
    bool getScopeMode() const  { return scopeMode; }

    // Reference overlay (pro mixing standards). w<0 → derive width heuristically.
    struct RefPoint { juce::String label; float x, y; float w = -1.f; };
    void setReference(juce::String name, std::vector<RefPoint> pts)
    { refName = std::move(name); refPoints = std::move(pts); repaint(); }

    juce::String getStatusText() const { return statusText; }

    std::function<void()>    onStatusChanged;
    std::function<void(int)> onTrackSelected;

private:
    std::vector<TrackEntry> tracks;
    int  dragId    = -1;
    int  selectedId = -1;
    int  lockedToId = -1;   // -1 = unlocked (any dot moveable)
    int  listenerId = -1;
    bool scopeMode  = false;
    int  focusedId  = -1;
    float animPhase = 0.f;
    juce::String statusText;

    // Zoom / pan (edit view)
    float              zoom = 1.f;
    juce::Point<float> pan  { 0.f, 0.f };
    bool               panning = false;
    juce::Point<float> lastPan;

    juce::AffineTransform viewT() const;
    juce::Point<float>    toWorld(juce::Point<float> screen) const;
    void                  clampPan();

    juce::Image scopeImg;   // phosphor persistence buffer

    juce::String          refName;
    std::vector<RefPoint> refPoints;

    void drawVectorscope(juce::Graphics& g);
    void drawReference(juce::Graphics& g);

    juce::Point<float> paramToPixel(float x, float y) const;
    void               pixelToParam(juce::Point<float> p, float& x, float& y) const;
    int                findTrackAt(juce::Point<float> p) const;

    void drawBackground(juce::Graphics& g);
    void drawCrosshairs(juce::Graphics& g, const TrackEntry& t);
    void drawTrack(juce::Graphics& g, const TrackEntry& t, bool selected, bool dimmed);
    void updateStatus();
};
