#include "MultiCanvas.h"

namespace { constexpr float kDotR = 9.f, kGrabR = 22.f; }

MultiCanvas::MultiCanvas()
{
    setMouseCursor(juce::MouseCursor::CrosshairCursor);
    listenerId = GlobalSpatialRegistry::get().addListener([this]() {
        tracks = GlobalSpatialRegistry::get().snapshot();
        updateStatus(); repaint();
    });
    tracks = GlobalSpatialRegistry::get().snapshot();
    startTimerHz(30);
}

MultiCanvas::~MultiCanvas()
{
    GlobalSpatialRegistry::get().removeListener(listenerId);
    stopTimer();
}

void MultiCanvas::timerCallback()
{
    tracks = GlobalSpatialRegistry::get().snapshot();
    animPhase += 0.055f;
    if (animPhase > juce::MathConstants<float>::twoPi)
        animPhase -= juce::MathConstants<float>::twoPi;
    repaint();
}

juce::Point<float> MultiCanvas::paramToPixel(float x, float y) const
{
    return { (x + 1.f) * 0.5f * getWidth(), (1.f - y) * getHeight() };
}
void MultiCanvas::pixelToParam(juce::Point<float> p, float& x, float& y) const
{
    x = juce::jlimit(-1.f, 1.f, p.x / getWidth()  * 2.f - 1.f);
    y = juce::jlimit(0.f,  1.f, 1.f - p.y / getHeight());
}
int MultiCanvas::findTrackAt(juce::Point<float> p) const
{
    if (lockedToId >= 0)
    {
        for (auto& t : tracks)
            if (t.id == lockedToId)
                return p.getDistanceFrom(paramToPixel(t.posX, t.posY)) < kGrabR
                       ? t.id : -1;
        return -1;
    }
    int best = -1; float bestD = kGrabR;
    for (auto& t : tracks) {
        float d = p.getDistanceFrom(paramToPixel(t.posX, t.posY));
        if (d < bestD) { bestD = d; best = t.id; }
    }
    return best;
}

void MultiCanvas::mouseDown(const juce::MouseEvent& e)
{
    dragId = findTrackAt(e.position);
    if (dragId >= 0 && dragId != selectedId) {
        selectedId = dragId;
        if (onTrackSelected) onTrackSelected(selectedId);
    }
    updateStatus(); repaint();
}
void MultiCanvas::mouseDrag(const juce::MouseEvent& e)
{
    if (dragId < 0) return;
    float x, y; pixelToParam(e.position, x, y);
    GlobalSpatialRegistry::get().setPosition(dragId, x, y);
}
void MultiCanvas::mouseUp(const juce::MouseEvent&) { dragId = -1; updateStatus(); }
void MultiCanvas::mouseDoubleClick(const juce::MouseEvent& e)
{
    int id = findTrackAt(e.position);
    if (id >= 0) GlobalSpatialRegistry::get().setPosition(id, 0.f, 0.f);
}

// ─── Drawing ──────────────────────────────────────────────────────────────────

void MultiCanvas::drawBackground(juce::Graphics& g)
{
    const float w = getWidth(), h = getHeight();

    // Deep navy base with vertical gradient
    juce::ColourGradient base(juce::Colour(0xff0a1426), 0.f, 0.f,
                               juce::Colour(0xff05080f), 0.f, h, false);
    g.setGradientFill(base);
    g.fillRect(getLocalBounds());

    // Subtle breathing radial glow centred on the listener (bottom-centre)
    const float breathe = 0.5f + 0.5f * std::sin(animPhase * 0.8f);
    juce::ColourGradient glow(
        juce::Colour(0xff16335f).withAlpha(0.14f + 0.06f * breathe), w*.5f, h*1.05f,
        juce::Colour(0x00000000), w*.5f, h*0.25f, true);
    g.setGradientFill(glow);
    g.fillRect(getLocalBounds());

    // Grid lines (subtle blue)
    g.setColour(juce::Colour(0xff1c3052));
    for (int i = 1; i < 12; ++i) g.drawLine(w*i/12.f, 0, w*i/12.f, h, 0.5f);
    for (int i = 1; i <  8; ++i) g.drawLine(0, h*i/8.f, w, h*i/8.f, 0.5f);

    // Centre axis — bright blue
    for (int k = 3; k >= 1; --k) {
        g.setColour(juce::Colour(0xff3a78d8).withAlpha(0.06f * k));
        g.drawLine(w*.5f, 0, w*.5f, h, 1.5f + k*2.f);
    }
    g.setColour(juce::Colour(0xff4a8ce8));
    g.drawLine(w*.5f, 0, w*.5f, h, 1.5f);

    // ── Full perspective depth arcs (complete semicircles) ────────────────────
    const float cx = w*.5f, cy = h;
    const float halfPi = juce::MathConstants<float>::halfPi;
    const int   nArcs  = 6;
    for (int i = 1; i <= nArcs; ++i) {
        const float rx = w * 0.5f * (float)i / nArcs;
        const float ry = (h * 0.96f) * (float)i / nArcs;
        juce::Path arc;
        // Full top semicircle: left (-90°) → top (0°) → right (+90°)
        arc.addCentredArc(cx, cy, rx, ry, 0.f, -halfPi, halfPi, true);

        // Soft glow (subtle)
        for (int k = 2; k >= 1; --k) {
            g.setColour(juce::Colour(0xff4a9cff).withAlpha(0.025f * k));
            g.strokePath(arc, juce::PathStrokeType(1.0f + k*1.4f));
        }
        // Core line — nearer arcs brighter
        const float coreA = juce::jlimit(0.18f, 0.55f, 0.20f + 0.07f * (nArcs - i));
        g.setColour(juce::Colour(0xff4f93e0).withAlpha(coreA));
        g.strokePath(arc, juce::PathStrokeType(1.0f));
    }

    // Distance labels along the centre (front→back)
    g.setFont(juce::Font(juce::FontOptions(7.5f).withStyle("Bold")));
    for (int i = 1; i <= nArcs - 1; ++i) {
        const float ay = cy - (h * 0.96f) * (float)i / nArcs;
        if (ay < 14) continue;
        g.setColour(juce::Colour(0xff3a6aa8).withAlpha(0.6f));
        g.drawText(juce::String(-i*3) + "dB", (int)(cx + 4), (int)(ay - 5), 34, 11,
                   juce::Justification::centredLeft);
    }

    // Edge labels (subtle)
    g.setFont(juce::Font(juce::FontOptions(8.5f).withStyle("Italic")));
    g.setColour(juce::Colour(0x40709ad0));
    g.drawText("L",     2,        (int)h/2-6, 12, 13, juce::Justification::centred);
    g.drawText("R",     (int)w-14,(int)h/2-6, 12, 13, juce::Justification::centred);
    g.drawText("BACK",  (int)w/2-16, 3,       32, 11, juce::Justification::centred);
    g.drawText("FRONT", (int)w/2-20,(int)h-13, 40, 11, juce::Justification::centred);
}

void MultiCanvas::drawCrosshairs(juce::Graphics& g, const TrackEntry& t)
{
    const float w = getWidth(), h = getHeight();
    auto dot = paramToPixel(t.posX, t.posY);
    const juce::Colour col = t.colour;

    // Fading crosshair lines
    for (float side : {-1.f, 1.f}) {
        float x0 = side < 0 ? 0.f : dot.x, x1 = side < 0 ? dot.x : w;
        juce::ColourGradient hg(col.withAlpha(side<0?.0f:.22f), x0, dot.y,
                                 col.withAlpha(side<0?.22f:.0f), x1, dot.y, false);
        g.setGradientFill(hg); g.fillRect(x0, dot.y-.6f, x1-x0, 1.2f);
    }
    for (float side : {-1.f, 1.f}) {
        float y0 = side < 0 ? 0.f : dot.y, y1 = side < 0 ? dot.y : h;
        juce::ColourGradient vg(col.withAlpha(side<0?.0f:.22f), dot.x, y0,
                                 col.withAlpha(side<0?.22f:.0f), dot.x, y1, false);
        g.setGradientFill(vg); g.fillRect(dot.x-.6f, y0, 1.2f, y1-y0);
    }
    g.setColour(col.withAlpha(0.5f));
    g.drawLine(dot.x-6.f,0.5f, dot.x+6.f,0.5f,1.5f);
    g.drawLine(dot.x-6.f,h-.5f,dot.x+6.f,h-.5f,1.5f);
    g.drawLine(0.5f,dot.y-6.f,0.5f,dot.y+6.f,1.5f);
    g.drawLine(w-.5f,dot.y-6.f,w-.5f,dot.y+6.f,1.5f);
}

void MultiCanvas::drawTrack(juce::Graphics& g, const TrackEntry& t,
                             bool selected, bool dimmed)
{
    auto dot = paramToPixel(t.posX, t.posY);
    const float px = dot.x, py = dot.y;
    const float r = selected ? kDotR + 1.f : kDotR;
    const float ml = t.meterL ? juce::jlimit(0.f,1.f,t.meterL->load()) : 0.f;
    const float mr = t.meterR ? juce::jlimit(0.f,1.f,t.meterR->load()) : 0.f;
    const float peak   = (ml + mr) * 0.5f;
    const float pulse  = selected ? (1.f + std::sin(animPhase) * 0.06f) : 1.f;
    const float spread = t.width * 36.f * pulse + peak * 8.f;
    const float ovalH  = 6.f + peak * 2.5f;
    const juce::Colour col = dimmed ? t.colour.withAlpha(0.25f) : t.colour;

    // ── Width indicator — bold, vivid glowing oval ────────────────────────────
    // Outer soft glow (wide blurred halo)
    for (int gi = 3; gi >= 1; --gi) {
        const float gExp = gi * 3.f;
        g.setColour(t.colour.withAlpha((uint8_t)(dimmed ? 4 : 14)));
        g.fillEllipse(px-spread-gExp, py-ovalH-gExp*0.5f,
                      (spread+gExp)*2.f, (ovalH+gExp*0.5f)*2.f);
    }

    // Strong radial fill
    juce::ColourGradient wg(t.colour.withAlpha(dimmed ? 0.10f : 0.45f + peak*0.18f), px, py,
                             t.colour.withAlpha(0.f), px - spread, py, true);
    g.setGradientFill(wg);
    g.fillEllipse(px-spread, py-ovalH, spread*2.f, ovalH*2.f);

    // Double outline ring — bright + inner highlight
    g.setColour(t.colour.brighter(0.5f).withAlpha(dimmed ? 0.30f : 1.0f));
    g.drawEllipse(px-spread, py-ovalH, spread*2.f, ovalH*2.f, selected?2.6f:1.8f);
    g.setColour(t.colour.brighter(0.9f).withAlpha(dimmed ? 0.15f : 0.6f));
    g.drawEllipse(px-spread+1.5f, py-ovalH+1.5f, (spread-1.5f)*2.f, (ovalH-1.5f)*2.f, 0.8f);

    // Bright horizontal axis through the oval
    g.setColour(t.colour.brighter(0.6f).withAlpha(dimmed?0.20f:0.85f));
    g.drawLine(px-spread, py, px+spread, py, selected?1.6f:1.1f);

    // Bold end-caps with arrows
    g.setColour(t.colour.brighter(0.7f).withAlpha(dimmed?0.30f:1.0f));
    for (float sx : {px-spread, px+spread}) {
        g.drawLine(sx, py-ovalH, sx, py+ovalH, selected?3.f:2.f);
        float dir = (sx < px) ? 1.f : -1.f;
        g.drawLine(sx, py, sx+dir*5.f, py-4.f, selected?2.2f:1.5f);
        g.drawLine(sx, py, sx+dir*5.f, py+4.f, selected?2.2f:1.5f);
    }

    if (dimmed) {
        // Simple diamond marker
        juce::Path d;
        d.addPolygon({px, py}, 4, r*0.85f, juce::MathConstants<float>::pi/4.f);
        g.setColour(col.withAlpha(0.45f));
        g.fillPath(d);
        return;
    }

    // ── Glow halo behind marker (subtle) ──────────────────────────────────────
    for (int i = selected?4:2; i >= 1; --i) {
        g.setColour(t.colour.withAlpha((uint8_t)((selected?16:8)*i)));
        g.fillEllipse(px-(r+i*4.f), py-(r+i*4.f), (r+i*4.f)*2.f, (r+i*4.f)*2.f);
    }

    // ── Marker: rotating diamond/star target ──────────────────────────────────
    const float rot = selected ? animPhase * 0.4f : 0.f;

    // Outer ring
    g.setColour(t.colour.withAlpha(selected?0.9f:0.55f));
    g.drawEllipse(px-r-2.f, py-r-2.f, (r+2.f)*2.f, (r+2.f)*2.f, selected?1.6f:1.1f);

    // Filled diamond core with gradient
    juce::Path diamond;
    diamond.addPolygon({px, py}, 4, r, rot + juce::MathConstants<float>::pi/4.f);
    juce::ColourGradient dg(t.colour.brighter(0.7f), px-r*.4f, py-r*.4f,
                             t.colour.darker(0.5f),   px+r*.4f, py+r*.4f, false);
    g.setGradientFill(dg);
    g.fillPath(diamond);

    // Diamond edge
    g.setColour(t.colour.brighter(0.3f).withAlpha(0.9f));
    g.strokePath(diamond, juce::PathStrokeType(1.f));

    // Bright centre pip (coloured, not pure white)
    g.setColour(t.colour.brighter(0.9f));
    g.fillEllipse(px-1.8f, py-1.8f, 3.6f, 3.6f);

    // Drop line
    g.setColour(t.colour.withAlpha((uint8_t)28));
    g.drawLine(px, py+r+5.f, px, getHeight(), 0.7f);

    // Label pill
    const float lx = juce::jlimit(2.f, getWidth()-54.f, px-24.f);
    const float ly = py+r+8.f;
    g.setColour(juce::Colour(0xcc050e1a));
    g.fillRoundedRectangle(lx-2.f, ly-1.f, 52.f, 13.f, 3.f);
    g.setColour(t.colour.withAlpha(0.4f));
    g.drawRoundedRectangle(lx-2.f, ly-1.f, 52.f, 13.f, 3.f, 0.6f);
    g.setFont(juce::Font(juce::FontOptions(9.5f).withStyle("Bold")));
    g.setColour(t.colour.brighter(0.5f));
    g.drawText(t.label, (int)lx, (int)ly, 52, 12, juce::Justification::centred);
}

void MultiCanvas::drawVectorscope(juce::Graphics& g)
{
    const int   W = getWidth(), H = getHeight();
    const float w = (float)W, h = (float)H;

    // Find focused track
    const TrackEntry* ft = nullptr;
    for (auto& t : tracks)
        if (t.id == focusedId) { ft = &t; break; }
    if (ft == nullptr && !tracks.empty()) ft = &tracks.front();

    const juce::Colour glowCol = ft ? ft->colour : juce::Colour(0xff39d4ff);

    // ── Phosphor persistence buffer ───────────────────────────────────────────
    if (scopeImg.isNull() || scopeImg.getWidth() != W || scopeImg.getHeight() != H)
        scopeImg = juce::Image(juce::Image::ARGB, juce::jmax(1, W), juce::jmax(1, H), true);

    const float cx    = w * 0.5f;
    const float baseY = h - 34.f;
    const float radius = juce::jmin(w * 0.44f, baseY - 24.f);
    const float scl   = radius * 1.0f;

    // Draw decayed trails + fresh points into the persistence image
    {
        juce::Graphics ig(scopeImg);

        // Fade previous frame (phosphor decay)
        ig.setColour(juce::Colours::black.withAlpha(0.18f));
        ig.fillRect(scopeImg.getBounds());

        if (ft && ft->scopeL && ft->scopeR && ft->scopePos && ft->scopeSize > 0)
        {
            const int      N  = ft->scopeSize;
            const uint32_t wp = ft->scopePos->load(std::memory_order_acquire);
            const juce::Colour hot  = glowCol.brighter(0.9f);
            const juce::Colour core = juce::Colour(0xffffffff);

            for (int i = 0; i < N; ++i)
            {
                const int idx = (int)((wp - 1 - (uint32_t)i) & (uint32_t)(N - 1));
                const float l = ft->scopeL[idx];
                const float r = ft->scopeR[idx];

                const float x = (-l + r) * 0.707f * scl;
                const float y = (-l - r) * 0.707f * scl;
                const float px = cx + x, py = baseY + y;

                const float age = 1.f - (float)i / (float)N;   // 1=newest

                // Wide soft glow
                ig.setColour(hot.withAlpha(0.06f + age * 0.10f));
                ig.fillEllipse(px - 3.4f, py - 3.4f, 6.8f, 6.8f);
                // Colored mid glow
                ig.setColour(hot.withAlpha(0.18f + age * 0.30f));
                ig.fillEllipse(px - 1.8f, py - 1.8f, 3.6f, 3.6f);
                // Bright hot core
                ig.setColour((age > 0.5f ? core : hot.brighter(0.5f)).withAlpha(0.55f + age * 0.45f));
                ig.fillEllipse(px - 0.9f, py - 0.9f, 1.8f, 1.8f);
            }
        }
    }

    // ── Composite: dark bg + phosphor image ───────────────────────────────────
    juce::ColourGradient bgGrad(juce::Colour(0xff05080f), cx, baseY,
                                 juce::Colour(0xff020308), 0.f, 0.f, true);
    g.setGradientFill(bgGrad);
    g.fillRect(getLocalBounds());

    g.drawImageAt(scopeImg, 0, 0);

    // ── Neon dome overlay (glow strokes) ──────────────────────────────────────
    auto neonArc = [&](float rr, juce::Colour c, float baseWidth) {
        juce::Path arc;
        arc.addCentredArc(cx, baseY, rr, rr, 0.f,
                          -juce::MathConstants<float>::pi,
                           0.f, true);
        // glow layers
        for (int k = 3; k >= 1; --k) {
            g.setColour(c.withAlpha(0.06f * k));
            g.strokePath(arc, juce::PathStrokeType(baseWidth + k * 2.2f));
        }
        g.setColour(c);
        g.strokePath(arc, juce::PathStrokeType(baseWidth));
    };

    // Inner amplitude rings (dim)
    for (int i = 1; i <= 3; ++i) {
        juce::Path ring;
        ring.addCentredArc(cx, baseY, radius*i/4.f, radius*i/4.f, 0.f,
                          -juce::MathConstants<float>::pi, 0.f, true);
        g.setColour(glowCol.withAlpha(0.10f));
        g.strokePath(ring, juce::PathStrokeType(0.7f));
    }

    // Main dome
    neonArc(radius, glowCol.withAlpha(0.85f), 1.4f);

    // Base line with glow
    for (int k = 3; k >= 1; --k) {
        g.setColour(glowCol.withAlpha(0.05f * k));
        g.drawLine(cx-radius, baseY, cx+radius, baseY, 1.4f + k*2.f);
    }
    g.setColour(glowCol.withAlpha(0.8f));
    g.drawLine(cx-radius, baseY, cx+radius, baseY, 1.2f);

    // Guide lines
    const float d = radius * 0.707f;
    g.setColour(glowCol.withAlpha(0.22f));
    g.drawLine(cx, baseY, cx, baseY - radius, 0.8f);   // mono
    g.drawLine(cx, baseY, cx - d, baseY - d, 0.8f);    // L
    g.drawLine(cx, baseY, cx + d, baseY - d, 0.8f);    // R

    // L / R / M labels (glowing)
    g.setFont(juce::Font(juce::FontOptions(10.f).withStyle("Bold")));
    g.setColour(glowCol.brighter(0.4f).withAlpha(0.9f));
    g.drawText("L", (int)(cx - d - 16), (int)(baseY - d - 7), 14, 14, juce::Justification::centred);
    g.drawText("R", (int)(cx + d + 2),  (int)(baseY - d - 7), 14, 14, juce::Justification::centred);
    g.drawText("M", (int)(cx - 7),      (int)(baseY - radius - 16), 14, 12, juce::Justification::centred);

    // Title
    g.setFont(juce::Font(juce::FontOptions(11.f).withStyle("Bold")));
    g.setColour(glowCol.brighter(0.3f));
    g.drawText(ft ? ("VECTORSCOPE — " + ft->label) : "VECTORSCOPE",
               10, 8, W - 20, 16, juce::Justification::centredLeft);

    // Width readout (how wide the field is)
    if (ft) {
        g.setFont(juce::Font(juce::FontOptions(9.f)));
        g.setColour(glowCol.withAlpha(0.6f));
        g.drawText("WIDTH " + juce::String(ft->width, 2),
                   W - 110, 8, 100, 16, juce::Justification::centredRight);
    }
}

void MultiCanvas::drawReference(juce::Graphics& g)
{
    if (refPoints.empty()) return;

    const float w = getWidth(), h = getHeight();
    const juce::Colour cyan(0xff66ccff);

    // Banner
    g.setFont(juce::Font(juce::FontOptions(9.5f).withStyle("Bold")));
    g.setColour(cyan.withAlpha(0.8f));
    g.drawText("REFERENCE: " + refName + "   (ghost = suggested position + width)",
               8, 6, (int)w - 16, 14, juce::Justification::centredLeft);

    for (auto& rp : refPoints)
    {
        const float px = (rp.x + 1.f) * 0.5f * w;
        const float py = (1.f - rp.y) * h;

        // Suggested width: explicit, else heuristic (centre/front = mono, sides/back = wide)
        const float width = rp.w >= 0.f
            ? rp.w
            : juce::jlimit(0.3f, 2.0f, 0.4f + std::abs(rp.x) * 1.1f + rp.y * 0.7f);

        // ── Ghost width oval (shows recommended stereo width) ─────────────────
        const float spread = width * 36.f;
        const float ovalH  = 5.f;
        g.setColour(cyan.withAlpha(0.07f));
        g.fillEllipse(px - spread, py - ovalH, spread * 2.f, ovalH * 2.f);
        g.setColour(cyan.withAlpha(0.35f));
        g.drawEllipse(px - spread, py - ovalH, spread * 2.f, ovalH * 2.f, 0.9f);
        // end caps
        g.setColour(cyan.withAlpha(0.4f));
        for (float sx : { px - spread, px + spread })
            g.drawLine(sx, py - ovalH, sx, py + ovalH, 0.9f);

        // Ghost marker ring
        g.setColour(cyan.withAlpha(0.45f));
        g.drawEllipse(px - 5.f, py - 5.f, 10.f, 10.f, 1.f);
        g.setColour(cyan.withAlpha(0.18f));
        g.fillEllipse(px - 4.f, py - 4.f, 8.f, 8.f);
        g.setColour(cyan.withAlpha(0.6f));
        g.drawLine(px - 3.f, py, px + 3.f, py, 0.8f);
        g.drawLine(px, py - 3.f, px, py + 3.f, 0.8f);

        // Label + width value
        g.setFont(juce::Font(juce::FontOptions(8.5f).withStyle("Bold")));
        g.setColour(cyan.withAlpha(0.75f));
        g.drawText(rp.label, (int)(px - 34), (int)(py + 9), 68, 11,
                   juce::Justification::centred);
        g.setFont(juce::Font(juce::FontOptions(7.5f)));
        g.setColour(cyan.withAlpha(0.5f));
        g.drawText("w " + juce::String(width, 1), (int)(px - 34), (int)(py + 19), 68, 10,
                   juce::Justification::centred);
    }
}

void MultiCanvas::paint(juce::Graphics& g)
{
    if (scopeMode) { drawVectorscope(g); return; }

    drawBackground(g);
    drawReference(g);

    if (selectedId >= 0)
        for (auto& t : tracks)
            if (t.id == selectedId) { drawCrosshairs(g, t); break; }

    auto sorted = tracks;
    std::sort(sorted.begin(), sorted.end(),
              [](const TrackEntry& a, const TrackEntry& b){ return a.posY > b.posY; });

    const bool anyLocked = lockedToId >= 0;
    const int  hlId      = lockedToId >= 0 ? lockedToId : selectedId;

    for (auto& t : sorted)
        drawTrack(g, t, t.id == hlId, anyLocked && t.id != hlId);
}

void MultiCanvas::updateStatus()
{
    const int hlId = lockedToId >= 0 ? lockedToId : selectedId;
    const TrackEntry* hl = nullptr;
    if (hlId >= 0)
        for (auto& t : tracks)
            if (t.id == hlId) { hl = &t; break; }

    if (hl != nullptr) {
        const juce::String ps = std::abs(hl->posX)<.01f ? "C"
            : hl->posX<0 ? "L"+juce::String((int)(std::abs(hl->posX)*100))
                         : "R"+juce::String((int)(hl->posX*100));
        statusText = hl->label+"  |  Pan: "+ps
            + "   Depth: "+juce::String((int)(hl->posY*100))
            + "%  ("+juce::String(-hl->posY*15.f,1)+" dB)"
            + "   Width: "+juce::String(hl->width,2);
    } else {
        statusText = juce::String(tracks.size())+" tracks — click to select, drag to position";
    }
    if (onStatusChanged) onStatusChanged();
}
