#pragma once
#include <JuceHeader.h>

class SpatialCanvas : public juce::Component
{
public:
    std::function<void(float posX, float posY)> onPositionChanged;

    SpatialCanvas();
    ~SpatialCanvas() override = default;

    void setPosition(float x, float y);  // x: -1..1, y: 0..1

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

private:
    float posX { 0.0f };   // -1..1  (left / right)
    float posY { 0.0f };   // 0..1   (front / back)
    bool  dragging { false };

    juce::Point<float> posToPixel() const;
    void updateFromMouse(juce::Point<int> mousePos);
};
