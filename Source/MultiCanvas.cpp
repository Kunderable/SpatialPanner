#include "MultiCanvas.h"

namespace { constexpr float kGrabR = 22.f; }

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

juce::AffineTransform MultiCanvas::viewT() const
{
    return juce::AffineTransform::scale(zoom).translated(pan.x, pan.y);
}
juce::Point<float> MultiCanvas::toWorld(juce::Point<float> screen) const
{
    return screen.transformedBy(viewT().inverted());
}
void MultiCanvas::clampPan()
{
    const float W = (float)getWidth(), H = (float)getHeight();
    if (zoom <= 1.f) { zoom = 1.f; pan = { 0.f, 0.f }; return; }
    pan.x = juce::jlimit(W * (1.f - zoom), 0.f, pan.x);
    pan.y = juce::jlimit(H * (1.f - zoom), 0.f, pan.y);
}

// paramToPixel / pixelToParam work in WORLD space (zoom applied via the paint transform)
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
    const auto world = toWorld(e.position);
    dragId = findTrackAt(world);
    if (dragId >= 0) {
        if (dragId != selectedId) {
            selectedId = dragId;
            if (onTrackSelected) onTrackSelected(selectedId);
        }
    } else if (zoom > 1.f) {
        // Empty space while zoomed -> pan
        panning = true;
        lastPan = e.position;
    }
    updateStatus(); repaint();
}
void MultiCanvas::mouseDrag(const juce::MouseEvent& e)
{
    if (dragId >= 0) {
        float x, y; pixelToParam(toWorld(e.position), x, y);
        GlobalSpatialRegistry::get().setPosition(dragId, x, y);
    } else if (panning) {
        pan += (e.position - lastPan);
        lastPan = e.position;
        clampPan();
        repaint();
    }
}
void MultiCanvas::mouseUp(const juce::MouseEvent&) { dragId = -1; panning = false; updateStatus(); }
void MultiCanvas::mouseDoubleClick(const juce::MouseEvent& e)
{
    const auto world = toWorld(e.position);
    int id = findTrackAt(world);
    if (id >= 0) {
        GlobalSpatialRegistry::get().setPosition(id, 0.f, 0.f);
    } else {
        // Double-click empty -> reset zoom
        zoom = 1.f; pan = { 0.f, 0.f };
        repaint();
    }
}
void MultiCanvas::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    if (scopeMode) return;
    const float delta = (w.deltaY != 0.f ? w.deltaY : w.deltaX);
    if (std::abs(delta) < 1e-4f) return;

    const auto worldUnder = toWorld(e.position);
    zoom = juce::jlimit(1.f, 8.f, zoom * (1.f + delta * 0.18f));
    // Keep the point under the cursor anchored
    pan = e.position - worldUnder * zoom;
    clampPan();
    repaint();
}

// Drawing

void MultiCanvas::drawBackground(juce::Graphics& g)
{
    const float w = getWidth(), h = getHeight();

    // Light stage with a soft warm vertical gradient
    juce::ColourGradient base(juce::Colour(0xffeeebe5), 0.f, 0.f,
                               juce::Colour(0xffe4e0d9), 0.f, h, false);
    g.setGradientFill(base);
    g.fillRect(getLocalBounds());

    // Gentle warm pool near the listener (bottom-centre)
    juce::ColourGradient pool(juce::Colour(0x14d9783f), w*.5f, h*1.02f,
                               juce::Colour(0x00d9783f), w*.5f, h*0.35f, true);
    g.setGradientFill(pool);
    g.fillRect(getLocalBounds());

    // Very subtle grid
    g.setColour(juce::Colour(0xffdad7d0).withAlpha(0.6f));
    for (int i = 1; i < 12; ++i) g.drawLine(w*i/12.f, 0, w*i/12.f, h, 0.5f);
    for (int i = 1; i <  8; ++i) g.drawLine(0, h*i/8.f, w, h*i/8.f, 0.5f);

    // Centre axis
    g.setColour(juce::Colour(0xffc8c4bc));
    g.drawLine(w*.5f, 0, w*.5f, h, 1.0f);

    // Thin concentric depth rings (semicircles from listener)
    const float cx = w*.5f, cy = h;
    const float halfPi = juce::MathConstants<float>::halfPi;
    const float maxR   = juce::jmin(w * 0.48f, h * 0.92f);
    const int   nArcs  = 5;
    for (int i = 1; i <= nArcs; ++i) {
        const float R = maxR * (float)i / (float)nArcs;
        juce::Path arc;
        arc.addCentredArc(cx, cy, R, R, 0.f, -halfPi, halfPi, true);
        g.setColour(juce::Colour(0xffceccc4));
        g.strokePath(arc, juce::PathStrokeType(0.8f));
    }

    // Edge labels
    g.setFont(juce::Font(juce::FontOptions(8.f).withStyle("Bold")));
    g.setColour(juce::Colour(0xff9a978f));
    g.drawText("L",     4,        (int)h/2-6, 12, 13, juce::Justification::centred);
    g.drawText("R",     (int)w-16,(int)h/2-6, 12, 13, juce::Justification::centred);
    g.drawText("BACK",  (int)w/2-16, 4,       32, 11, juce::Justification::centred);
    g.drawText("FRONT", (int)w/2-20,(int)h-14, 40, 11, juce::Justification::centred);
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
    const float r = selected ? 6.f : 5.f;
    const float spread = t.width * 36.f;
    const float ovalH  = 5.f;
    const juce::Colour col = dimmed ? t.colour.withAlpha(0.28f) : t.colour;

    // Width indicator: thin clean oval
    g.setColour(col.withAlpha(dimmed ? 0.06f : 0.12f));
    g.fillEllipse(px-spread, py-ovalH, spread*2.f, ovalH*2.f);
    g.setColour(col.withAlpha(dimmed ? 0.25f : selected ? 0.9f : 0.55f));
    g.drawEllipse(px-spread, py-ovalH, spread*2.f, ovalH*2.f, selected ? 1.4f : 1.0f);
    // thin end caps
    for (float sx : {px-spread, px+spread})
        g.drawLine(sx, py-ovalH, sx, py+ovalH, selected ? 1.2f : 0.9f);

    if (dimmed) {
        g.setColour(col.withAlpha(0.5f));
        g.fillEllipse(px-r*0.7f, py-r*0.7f, r*1.4f, r*1.4f);
        return;
    }

    // Soft drop shadow for depth
    g.setColour(juce::Colour(0x18000000));
    g.fillEllipse(px-r+0.6f, py-r+1.4f, r*2.f, r*2.f);

    // Selection ring
    if (selected) {
        g.setColour(col.withAlpha(0.45f));
        g.drawEllipse(px-r-3.f, py-r-3.f, (r+3.f)*2.f, (r+3.f)*2.f, 1.2f);
    }

    // Clean filled dot (donut on light bg) with a soft top highlight
    juce::ColourGradient dg(col.brighter(0.18f), px, py-r,
                             col.darker(0.12f),  px, py+r, false);
    g.setGradientFill(dg);
    g.fillEllipse(px-r, py-r, r*2.f, r*2.f);
    g.setColour(juce::Colour(0xffeeebe5));
    g.fillEllipse(px-r*0.4f, py-r*0.4f, r*0.8f, r*0.8f);
    g.setColour(col);
    g.fillEllipse(px-r*0.2f, py-r*0.2f, r*0.4f, r*0.4f);

    // Label
    const float lx = juce::jlimit(2.f, getWidth()-54.f, px-24.f);
    const float ly = py+r+5.f;
    g.setFont(juce::Font(juce::FontOptions(9.f).withStyle("Bold")));
    g.setColour(col.withAlpha(0.85f));
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

    // Phosphor persistence buffer
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

        if (ft && ft->alive && ft->alive->load()
            && ft->scopeL && ft->scopeR && ft->scopePos && ft->scopeSize > 0)
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

    // Composite: dark bg + phosphor image
    juce::ColourGradient bgGrad(juce::Colour(0xff05080f), cx, baseY,
                                 juce::Colour(0xff020308), 0.f, 0.f, true);
    g.setGradientFill(bgGrad);
    g.fillRect(getLocalBounds());

    g.drawImageAt(scopeImg, 0, 0);

    // Neon dome overlay (glow strokes)
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
    const juce::Colour cyan(0xff2f8a9c);

    // Banner
    g.setFont(juce::Font(juce::FontOptions(9.5f).withStyle("Bold")));
    g.setColour(cyan.withAlpha(0.9f));
    g.drawText("REFERENCE: " + refName + "   (ghost = suggested position + width)",
               8, 6, (int)w - 16, 14, juce::Justification::centredLeft);

    // Merge "X L"/"X R" pairs into a single wide centred element ("X <>")
    std::vector<RefPoint> merged;
    std::vector<bool> used(refPoints.size(), false);
    for (size_t i = 0; i < refPoints.size(); ++i)
    {
        if (used[i]) continue;
        const auto& a = refPoints[i];
        bool paired = false;
        if (a.label.endsWith(" L") || a.label.endsWith(" R"))
        {
            const juce::String base = a.label.dropLastCharacters(2);
            const juce::String other = base + (a.label.endsWith(" L") ? " R" : " L");
            for (size_t j = i + 1; j < refPoints.size(); ++j)
            {
                if (used[j] || refPoints[j].label != other) continue;
                const auto& b = refPoints[j];
                const float spreadX = juce::jmax(std::abs(a.x), std::abs(b.x));
                RefPoint m;
                m.label = base + " <>";
                m.x = 0.f;
                m.y = (a.y + b.y) * 0.5f;
                m.w = juce::jlimit(0.3f, 2.0f, spreadX * 2.0f + 0.5f);
                merged.push_back(m);
                used[i] = used[j] = true;
                paired = true;
                break;
            }
        }
        if (!paired) { merged.push_back(a); used[i] = true; }
    }

    struct Ghost { float px, py, spread, width; juce::String label; };
    std::vector<Ghost> ghosts;
    ghosts.reserve(merged.size());

    // First: draw all markers/ovals, collect label anchors
    for (auto& rp : merged)
    {
        const float px = (rp.x + 1.f) * 0.5f * w;
        const float py = (1.f - rp.y) * h;
        const float width = rp.w >= 0.f
            ? rp.w
            : juce::jlimit(0.3f, 2.0f, 0.4f + std::abs(rp.x) * 1.1f + rp.y * 0.7f);
        const float spread = width * 36.f;
        const float ovalH  = 5.f;

        g.setColour(cyan.withAlpha(0.07f));
        g.fillEllipse(px - spread, py - ovalH, spread * 2.f, ovalH * 2.f);
        g.setColour(cyan.withAlpha(0.35f));
        g.drawEllipse(px - spread, py - ovalH, spread * 2.f, ovalH * 2.f, 0.9f);
        g.setColour(cyan.withAlpha(0.4f));
        for (float sx : { px - spread, px + spread })
            g.drawLine(sx, py - ovalH, sx, py + ovalH, 0.9f);

        g.setColour(cyan.withAlpha(0.5f));
        g.drawEllipse(px - 5.f, py - 5.f, 10.f, 10.f, 1.f);
        g.setColour(cyan.withAlpha(0.20f));
        g.fillEllipse(px - 4.f, py - 4.f, 8.f, 8.f);
        g.setColour(cyan.withAlpha(0.7f));
        g.drawLine(px - 3.f, py, px + 3.f, py, 0.8f);
        g.drawLine(px, py - 3.f, px, py + 3.f, 0.8f);

        ghosts.push_back({ px, py, spread, width, rp.label });
    }

    // Second: place single-line labels with anti-overlap (push down on collision)
    const int   lw = 84, lh = 12;
    std::vector<juce::Rectangle<int>> placed;
    g.setFont(juce::Font(juce::FontOptions(8.5f).withStyle("Bold")));

    // Draw nearer (front, larger py) labels first so they win the lower rows
    std::sort(ghosts.begin(), ghosts.end(),
              [](const Ghost& a, const Ghost& b){ return a.py > b.py; });

    for (auto& gh : ghosts)
    {
        int lx = juce::jlimit(2, (int)w - lw - 2, (int)(gh.px - lw / 2));
        int ly = (int)(gh.py + 8);
        juce::Rectangle<int> r(lx, ly, lw, lh);

        // Nudge down while overlapping an already-placed label
        bool moved = true; int guard = 0;
        while (moved && guard++ < 40) {
            moved = false;
            for (auto& p : placed)
                if (r.intersects(p)) { r.translate(0, lh + 1); moved = true; break; }
        }
        placed.push_back(r);

        // Connector line if the label drifted away from its marker
        if (r.getY() > gh.py + 14) {
            g.setColour(cyan.withAlpha(0.25f));
            g.drawLine(gh.px, gh.py + 6.f, (float)r.getCentreX(), (float)r.getY(), 0.6f);
        }

        g.setColour(cyan.withAlpha(0.95f));
        g.drawText(gh.label + "  " + juce::String(gh.width, 1),
                   r, juce::Justification::centred);
    }
}

void MultiCanvas::paint(juce::Graphics& g)
{
    if (scopeMode) { drawVectorscope(g); return; }

    // Apply zoom/pan to the whole scene
    g.saveState();
    g.addTransform(viewT());

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

    g.restoreState();

    // Zoom indicator (screen space, not transformed)
    if (zoom > 1.01f) {
        g.setFont(juce::Font(juce::FontOptions(9.5f).withStyle("Bold")));
        g.setColour(juce::Colour(0xff2f8a9c).withAlpha(0.9f));
        g.drawText(juce::String(zoom, 1) + "x   (drag empty = pan · dbl-click = reset)",
                   8, getHeight() - 18, getWidth() - 16, 14,
                   juce::Justification::centredLeft);
    }

    // Outer border on top
    g.setColour(juce::Colour(0xffd2cfc8));
    g.drawRect(getLocalBounds(), 1);
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
