#include "SpatialCanvas.h"

SpatialCanvas::SpatialCanvas()
{
    setMouseCursor(juce::MouseCursor::CrosshairCursor);
}

void SpatialCanvas::setPosition(float x, float y)
{
    posX = juce::jlimit(-1.0f, 1.0f, x);
    posY = juce::jlimit(0.0f, 1.0f, y);
    repaint();
}

juce::Point<float> SpatialCanvas::posToPixel() const
{
    float px = (posX + 1.0f) * 0.5f * (float)getWidth();
    float py = (1.0f - posY) * (float)getHeight();  // posY=0 (front) → bottom
    return { px, py };
}

void SpatialCanvas::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    float w = bounds.getWidth();
    float h = bounds.getHeight();

    // Background gradient: darker at top (back/far), slightly warmer at bottom (front)
    juce::ColourGradient bg(
        juce::Colour(0xff080514), 0.0f, 0.0f,
        juce::Colour(0xff130c28), 0.0f, h,
        false);
    g.setGradientFill(bg);
    g.fillRect(bounds);

    // Fine grid
    g.setColour(juce::Colour(0xff1e1538));
    for (int i = 1; i < 10; ++i)
    {
        float x = w * (float)i / 10.0f;
        float y = h * (float)i / 10.0f;
        g.drawLine(x, 0.0f, x, h, 0.5f);
        g.drawLine(0.0f, y, w, y, 0.5f);
    }

    // Centre axis (brighter)
    g.setColour(juce::Colour(0xff3d2870));
    g.drawLine(w * 0.5f, 0.0f, w * 0.5f, h, 1.0f);

    // Perspective depth arcs from listener at bottom-centre
    float lx = w * 0.5f;
    float ly = h + 10.0f;  // listener just off the bottom edge
    for (int i = 1; i <= 5; ++i)
    {
        float t = (float)i / 5.0f;
        float rx = w * t * 0.5f;
        float ry = h * t * 0.7f;
        g.setColour(juce::Colour::fromRGBA(100, 60, 180, (uint8_t)(18 + i * 4)));
        g.drawEllipse(lx - rx, ly - ry, rx * 2.0f, ry * 2.0f, 0.5f);
    }

    // Edge labels
    g.setFont(juce::Font(juce::FontOptions(9.5f).withStyle("Italic")));
    g.setColour(juce::Colour(0x556a4db0));
    g.drawText("L",     2,        (int)h / 2 - 7, 14, 14, juce::Justification::centred);
    g.drawText("R",     (int)w - 16, (int)h / 2 - 7, 14, 14, juce::Justification::centred);
    g.drawText("BACK",  (int)w / 2 - 18, 3,         36, 12, juce::Justification::centred);
    g.drawText("FRONT", (int)w / 2 - 20, (int)h - 14, 40, 12, juce::Justification::centred);

    // ---- Dot ----
    auto dot = posToPixel();

    // Glow rings
    const juce::Colour glowColor(0xffa855f7);
    for (int i = 5; i >= 1; --i)
    {
        float r = 5.0f + i * 5.5f;
        g.setColour(glowColor.withAlpha((uint8_t)(12 * i)));
        g.fillEllipse(dot.x - r, dot.y - r, r * 2.0f, r * 2.0f);
    }

    // Core dot
    float dotR = 7.5f;
    juce::ColourGradient dotGrad(
        juce::Colour(0xfff5d0fe), dot.x - dotR * 0.3f, dot.y - dotR * 0.4f,
        juce::Colour(0xff8b5cf6), dot.x + dotR * 0.3f, dot.y + dotR * 0.4f,
        false);
    g.setGradientFill(dotGrad);
    g.fillEllipse(dot.x - dotR, dot.y - dotR, dotR * 2.0f, dotR * 2.0f);

    // Specular
    g.setColour(juce::Colour(0x70ffffff));
    g.fillEllipse(dot.x - dotR * 0.35f, dot.y - dotR * 0.55f,
                  dotR * 0.55f, dotR * 0.4f);

    // Drop line to bottom
    g.setColour(glowColor.withAlpha((uint8_t)60));
    g.drawLine(dot.x, dot.y + dotR, dot.x, h, 1.0f);

    // Border
    g.setColour(juce::Colour(0xff2e1f58));
    g.drawRect(bounds, 1.0f);
}

void SpatialCanvas::mouseDown(const juce::MouseEvent& e)
{
    dragging = true;
    updateFromMouse(e.getPosition());
}

void SpatialCanvas::mouseDrag(const juce::MouseEvent& e)
{
    if (dragging)
        updateFromMouse(e.getPosition());
}

void SpatialCanvas::mouseUp(const juce::MouseEvent& e)
{
    juce::ignoreUnused(e);
    dragging = false;
}

void SpatialCanvas::updateFromMouse(juce::Point<int> p)
{
    float w = (float)getWidth();
    float h = (float)getHeight();
    if (w <= 0.0f || h <= 0.0f)
        return;

    posX = juce::jlimit(-1.0f, 1.0f, ((float)p.x / w) * 2.0f - 1.0f);
    posY = juce::jlimit(0.0f,  1.0f, 1.0f - (float)p.y / h);

    repaint();

    if (onPositionChanged)
        onPositionChanged(posX, posY);
}
