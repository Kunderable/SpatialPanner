#include "PluginEditor.h"
#include "GenreData.h"

// ════════════════════════════════════════════════════════════════════════════
//  GenrePanel — animated modal genre picker
// ════════════════════════════════════════════════════════════════════════════
namespace {
    const juce::Colour kgGlass   { 0xf2101424 };
    const juce::Colour kgCard    { 0xff141a2e };
    const juce::Colour kgChip    { 0xff1d2640 };
    const juce::Colour kgChipHi  { 0xff3a6ad0 };
    const juce::Colour kgAccent  { 0xff5aa8ff };
    const juce::Colour kgText    { 0xffe0e8ff };
    const juce::Colour kgDim     { 0xff7088b8 };
}

GenrePanel::GenrePanel()
{
    setInterceptsMouseClicks(true, true);
    buildChips();
}

void GenrePanel::buildChips()
{
    chips.clear();
    for (auto& g : GenreData::list())
        chips.push_back({ g.category, g.name, {} });
}

void GenrePanel::showPanel()
{
    open = true; closing = false;
    setVisible(true);
    toFront(false);
    startTimerHz(60);
}

void GenrePanel::hidePanel()
{
    closing = true;
    startTimerHz(60);
}

void GenrePanel::timerCallback()
{
    const float target = closing ? 0.f : 1.f;
    anim += (target - anim) * 0.25f;
    if (std::abs(target - anim) < 0.01f) {
        anim = target;
        stopTimer();
        if (closing) { open = false; setVisible(false); if (onClosed) onClosed(); }
    }
    resized();
    repaint();
}

void GenrePanel::resized()
{
    // Card centred, sized to content
    const int catCount = 2;
    const int perRow   = 4;
    const int chipW = 124, chipH = 34, gap = 10;
    const int rowsE = 3, rowsB = 2;  // electronic 11→3 rows, band 8→2 rows

    const int cardW = perRow * chipW + (perRow + 1) * gap;
    const int cardH = 70 + (rowsE + rowsB) * (chipH + gap) + catCount * 30 + 30;

    cardBounds = { (getWidth() - cardW) / 2, (getHeight() - cardH) / 2, cardW, cardH };

    // Lay out chips by category
    int x = cardBounds.getX() + gap;
    int y = cardBounds.getY() + 64;
    juce::String lastCat;
    int col = 0;
    for (auto& c : chips)
    {
        if (c.cat != lastCat) {
            lastCat = c.cat;
            if (col != 0) { y += chipH + gap; col = 0; }
            y += 28;  // category header space
            x = cardBounds.getX() + gap;
        }
        c.bounds = { x, y, chipW, chipH };
        x += chipW + gap;
        if (++col >= perRow) { col = 0; x = cardBounds.getX() + gap; y += chipH + gap; }
    }
}

int GenrePanel::chipAt(juce::Point<int> p) const
{
    for (int i = 0; i < (int)chips.size(); ++i)
        if (chips[i].bounds.contains(p)) return i;
    return -1;
}

void GenrePanel::mouseMove(const juce::MouseEvent& e)
{
    const int h = chipAt(e.getPosition());
    if (h != hoverIdx) { hoverIdx = h; repaint(); }
}

void GenrePanel::mouseDown(const juce::MouseEvent& e)
{
    const int idx = chipAt(e.getPosition());
    if (idx >= 0) {
        if (onPick) onPick(chips[idx].name);
        hidePanel();
        return;
    }
    // Click outside the card → close (clear)
    if (!cardBounds.contains(e.getPosition()))
        hidePanel();
}

void GenrePanel::paint(juce::Graphics& g)
{
    const float a = juce::jlimit(0.f, 1.f, anim);

    // Dim backdrop
    g.fillAll(kgGlass.withAlpha(0.0f + 0.82f * a));

    // Card with scale-in animation
    auto card = cardBounds.toFloat();
    const float scale = 0.94f + 0.06f * a;
    auto cx = card.getCentreX(), cy = card.getCentreY();
    card = card.withSizeKeepingCentre(card.getWidth() * scale, card.getHeight() * scale);
    juce::ignoreUnused(cx, cy);

    g.setOpacity(a);

    // Card shadow
    for (int k = 8; k >= 1; --k) {
        g.setColour(juce::Colours::black.withAlpha(0.04f * a));
        g.fillRoundedRectangle(card.expanded((float)k), 16.f);
    }

    // Card body
    juce::ColourGradient cg(kgCard.brighter(0.06f), card.getX(), card.getY(),
                             kgCard.darker(0.12f),  card.getX(), card.getBottom(), false);
    g.setGradientFill(cg);
    g.fillRoundedRectangle(card, 14.f);
    g.setColour(kgAccent.withAlpha(0.25f));
    g.drawRoundedRectangle(card, 14.f, 1.f);

    // Top accent glow line
    g.setColour(kgAccent.withAlpha(0.6f));
    g.fillRoundedRectangle(card.getX() + 20, card.getY() + 2, card.getWidth() - 40, 2.f, 1.f);

    // Title
    g.setFont(juce::Font(juce::FontOptions(17.f).withStyle("Bold")));
    g.setColour(kgText);
    g.drawText("CHOOSE A REFERENCE", (int)card.getX() + 24, (int)card.getY() + 16,
               (int)card.getWidth() - 48, 24, juce::Justification::centredLeft);
    g.setFont(juce::Font(juce::FontOptions(9.f)));
    g.setColour(kgDim);
    g.drawText("Pro mixing layouts · click to apply · click outside to cancel",
               (int)card.getX() + 24, (int)card.getY() + 38,
               (int)card.getWidth() - 48, 16, juce::Justification::centredLeft);

    // Category headers + chips
    juce::String lastCat;
    for (int i = 0; i < (int)chips.size(); ++i)
    {
        const auto& c = chips[i];
        if (c.cat != lastCat) {
            lastCat = c.cat;
            g.setFont(juce::Font(juce::FontOptions(9.5f).withStyle("Bold")));
            g.setColour(kgAccent.withAlpha(0.8f));
            g.drawText(c.cat, c.bounds.getX(), c.bounds.getY() - 22,
                       300, 16, juce::Justification::centredLeft);
        }

        auto cb = c.bounds.toFloat();
        const bool hov = (i == hoverIdx);
        // chip body
        juce::ColourGradient chg(hov ? kgChipHi.brighter(0.1f) : kgChip.brighter(0.05f),
                                  cb.getX(), cb.getY(),
                                  hov ? kgChipHi.darker(0.2f) : kgChip.darker(0.1f),
                                  cb.getX(), cb.getBottom(), false);
        g.setGradientFill(chg);
        g.fillRoundedRectangle(cb, 8.f);
        if (hov) {
            g.setColour(kgAccent.withAlpha(0.5f));
            g.drawRoundedRectangle(cb, 8.f, 1.4f);
        } else {
            g.setColour(juce::Colours::white.withAlpha(0.05f));
            g.drawRoundedRectangle(cb, 8.f, 0.8f);
        }
        g.setFont(juce::Font(juce::FontOptions(11.5f).withStyle(hov ? "Bold" : "Regular")));
        g.setColour(hov ? juce::Colours::white : kgText.withAlpha(0.85f));
        g.drawText(c.name, c.bounds, juce::Justification::centred);
    }

    g.setOpacity(1.f);
}

// ════════════════════════════════════════════════════════════════════════════

// ─── S1-inspired design constants ─────────────────────────────────────────────
namespace
{
    // Dimensions
    constexpr int kW       = 660;
    constexpr int kH       = 560;
    constexpr int kTitleH  = 38;
    constexpr int kListW   = 116;
    constexpr int kMeterW  = 82;
    constexpr int kWidthH  = 44;
    constexpr int kBottomH = 28;
    constexpr int kStatusH = 20;

    constexpr int kMainY   = kTitleH;
    constexpr int kMainH   = kH - kTitleH - kWidthH - kBottomH - kStatusH;
    constexpr int kCanvasX = kListW + 1;
    constexpr int kCanvasW = kW - kListW - kMeterW - 2;
    constexpr int kMeterX  = kW - kMeterW;

    // S1-inspired grey metal palette
    const juce::Colour kBg       { 0xff2d2d2d };   // overall body
    const juce::Colour kPanel    { 0xff363636 };   // raised panels
    const juce::Colour kDark     { 0xff1c1c1c };   // dark separators
    const juce::Colour kHighlt   { 0xff525252 };   // bevel highlight
    const juce::Colour kScreen   { 0xff08101e };   // scope background (dark navy)
    const juce::Colour kText     { 0xffcccccc };   // normal text
    const juce::Colour kDim      { 0xff888888 };   // dimmer text / labels
    const juce::Colour kBlue     { 0xff4477cc };   // accent blue (like S1 scope)
    const juce::Colour kBorder   { 0xff1a1a1a };

    static const char*        kEnvNames[4]   = { "Studio", "Club", "Car", "Phone" };
    static const juce::Colour kEnvOn[4]      = {
        juce::Colour(0xff44aaaa), juce::Colour(0xffcc4444),
        juce::Colour(0xffccaa22), juce::Colour(0xff4466cc),
    };

    juce::Colour meterCol(float norm)
    {
        if (norm > 0.92f) return juce::Colour(0xffee2222);
        if (norm > 0.75f) return juce::Colour(0xffeebb11);
        return juce::Colour(0xff33cc33);
    }

    // ── Bevel / inset helpers ─────────────────────────────────────────────────
    void fillPanel(juce::Graphics& g, juce::Rectangle<int> r)
    {
        // Subtle top-to-bottom gradient for "raised panel" feel
        juce::ColourGradient grad(kPanel.brighter(0.04f), (float)r.getX(), (float)r.getY(),
                                   kPanel.darker(0.04f),  (float)r.getX(), (float)r.getBottom(), false);
        g.setGradientFill(grad);
        g.fillRect(r);
    }

    void drawBevel(juce::Graphics& g, juce::Rectangle<int> r, bool inset = false)
    {
        // Top/left highlight, bottom/right shadow for raised look (reverse for inset)
        const juce::Colour hi = inset ? kBorder  : kHighlt;
        const juce::Colour sh = inset ? kHighlt  : kBorder;
        g.setColour(hi);
        g.drawLine((float)r.getX(),     (float)r.getY(),      (float)r.getRight(),(float)r.getY(),     1.f);
        g.drawLine((float)r.getX(),     (float)r.getY(),      (float)r.getX(),   (float)r.getBottom(),1.f);
        g.setColour(sh);
        g.drawLine((float)r.getX(),     (float)r.getBottom(), (float)r.getRight(),(float)r.getBottom(),1.f);
        g.drawLine((float)r.getRight(), (float)r.getY(),      (float)r.getRight(),(float)r.getBottom(),1.f);
    }

    void drawScreenInset(juce::Graphics& g, juce::Rectangle<int> r)
    {
        // Inset "screen" frame: outer raised bezel, inner screen fill
        auto outer = r;
        fillPanel(g, outer.expanded(2));
        drawBevel(g, outer.expanded(2), false);  // outer raised bezel
        g.setColour(kScreen);
        g.fillRect(r);
        drawBevel(g, r, true);                   // inner inset shadow
    }
}

// ─── Constructor ──────────────────────────────────────────────────────────────
SpatialPannerAudioProcessorEditor::SpatialPannerAudioProcessorEditor(
    SpatialPannerAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    setResizable(true, true);
    if (auto* c = getConstrainer()) {
        c->setFixedAspectRatio((double)kW / (double)kH);
        c->setSizeLimits(kW * 3 / 4, kH * 3 / 4, kW * 2, kH * 2);
    }
    setSize(kW, kH);
    focusedTrackId = processorRef.registryId;

    // ── Track list ────────────────────────────────────────────────────────────
    addAndMakeVisible(trackList);
    trackList.refresh();
    trackList.onSelect = [this](int id) {
        canvas.setLockedTrackId(id);
        focusedTrackId = id >= 0 ? id : processorRef.registryId;
        canvas.setFocusedId(focusedTrackId);
        peakLDisplay = peakRDisplay = 0.f; peakHoldFrames = 0;
        peakLatchL = peakLatchR = 0.f;
    };

    // ── Canvas ────────────────────────────────────────────────────────────────
    addAndMakeVisible(canvas);
    canvas.onStatusChanged = [this]() {
        statusLabel.setText(canvas.getStatusText(), juce::dontSendNotification);
    };
    canvas.onTrackSelected = [this](int id) {
        focusedTrackId = id;
        canvas.setFocusedId(id);
        peakLDisplay = peakRDisplay = 0.f; peakHoldFrames = 0;
        peakLatchL = peakLatchR = 0.f;
        trackList.setSelectedId(id);
    };

    // ── WIDTH slider (big, at bottom) ─────────────────────────────────────────
    widthSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    widthSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 56, 22);
    // S1-style slider colours
    widthSlider.setColour(juce::Slider::backgroundColourId,        juce::Colour(0xff1e1e1e));
    widthSlider.setColour(juce::Slider::thumbColourId,             juce::Colour(0xff888888));
    widthSlider.setColour(juce::Slider::trackColourId,             kBlue.darker(0.4f));
    widthSlider.setColour(juce::Slider::textBoxTextColourId,        kText);
    widthSlider.setColour(juce::Slider::textBoxBackgroundColourId,  juce::Colour(0xff222222));
    widthSlider.setColour(juce::Slider::textBoxOutlineColourId,     kBorder);
    widthSlider.setRange(0.0, 2.0, 0.001);
    widthSlider.setValue(processorRef.apvts.getRawParameterValue("width")->load(),
                         juce::dontSendNotification);
    addAndMakeVisible(widthSlider);
    // Width slider controls the SELECTED track (via registry), with sync back
    widthSlider.onValueChange = [this]() {
        GlobalSpatialRegistry::get().setWidth(focusedTrackId, (float)widthSlider.getValue());
    };

    // ── Name editor ───────────────────────────────────────────────────────────
    nameEditor.setMultiLine(false);
    nameEditor.setText(processorRef.getTrackName(), false);
    nameEditor.setTextToShowWhenEmpty("Track name…", kDim);
    nameEditor.setFont(juce::Font(juce::FontOptions(11.f)));
    nameEditor.setColour(juce::TextEditor::backgroundColourId,     juce::Colour(0xff202020));
    nameEditor.setColour(juce::TextEditor::textColourId,           kText);
    nameEditor.setColour(juce::TextEditor::outlineColourId,        kBorder);
    nameEditor.setColour(juce::TextEditor::focusedOutlineColourId, kBlue);
    nameEditor.setColour(juce::TextEditor::highlightColourId,      kBlue.withAlpha(.35f));
    nameEditor.onTextChange = [this]() { processorRef.setTrackName(nameEditor.getText()); };
    addAndMakeVisible(nameEditor);

    // ── Environment preset buttons ─────────────────────────────────────────────
    for (int i = 0; i < 4; ++i)
    {
        envBtns[i].setButtonText(kEnvNames[i]);
        // S1-style pushbutton look
        envBtns[i].setColour(juce::TextButton::buttonColourId,   juce::Colour(0xff323232));
        envBtns[i].setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff3a3a3a));
        envBtns[i].setColour(juce::TextButton::textColourOffId,  kDim);
        envBtns[i].setColour(juce::TextButton::textColourOnId,   kText);
        envBtns[i].onClick = [this, i]() {
            const int newMode = (activeEnvIdx == i+1) ? 0 : (i+1);
            activeEnvIdx = newMode;
            processorRef.requestEnvironment(newMode);
            updateEnvButtons();
        };
        addAndMakeVisible(envBtns[i]);
    }
    activeEnvIdx = processorRef.getEnvironmentMode();
    updateEnvButtons();

    // ── Vectorscope toggle ─────────────────────────────────────────────────────
    scopeBtn.setButtonText("SCOPE");
    scopeBtn.setColour(juce::TextButton::buttonColourId,   juce::Colour(0xff2e2e2e));
    scopeBtn.setColour(juce::TextButton::buttonOnColourId, kBlue.darker(0.3f));
    scopeBtn.setColour(juce::TextButton::textColourOffId,  kDim);
    scopeBtn.setColour(juce::TextButton::textColourOnId,   kText);
    scopeBtn.setClickingTogglesState(false);
    scopeBtn.onClick = [this]() {
        scopeOn = !scopeOn;
        canvas.setScopeMode(scopeOn);
        scopeBtn.setColour(juce::TextButton::buttonColourId,
                           scopeOn ? kBlue.darker(0.3f) : juce::Colour(0xff2e2e2e));
        scopeBtn.setColour(juce::TextButton::textColourOffId,
                           scopeOn ? kText : kDim);
    };
    addAndMakeVisible(scopeBtn);

    // ── Reference layouts ───────────────────────────────────────────────────────
    refBtn.setButtonText("REF");
    refBtn.setColour(juce::TextButton::buttonColourId,   juce::Colour(0xff2e2e2e));
    refBtn.setColour(juce::TextButton::textColourOffId,  kDim);
    refBtn.setClickingTogglesState(false);
    refBtn.onClick = [this]() {
        if (genrePanel.isOpen()) genrePanel.hidePanel();
        else                     genrePanel.showPanel();
    };
    addAndMakeVisible(refBtn);

    // ── Genre picker overlay ────────────────────────────────────────────────────
    addChildComponent(genrePanel);
    genrePanel.onPick = [this](juce::String genre) {
        refGenre = genre;
        applyReference(genre);
        const bool on = genre.isNotEmpty();
        refBtn.setButtonText(on ? genre.substring(0, 6).toUpperCase() : "REF");
        refBtn.setColour(juce::TextButton::buttonColourId,
                         on ? juce::Colour(0xff2a5a8a) : juce::Colour(0xff2e2e2e));
        refBtn.setColour(juce::TextButton::textColourOffId, on ? kText : kDim);
    };

    // ── Status ────────────────────────────────────────────────────────────────
    statusLabel.setFont(juce::Font(juce::FontOptions(9.5f)));
    statusLabel.setColour(juce::Label::textColourId, kDim);
    statusLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(statusLabel);

    canvas.setFocusedId(focusedTrackId);

    startTimerHz(25);
}

SpatialPannerAudioProcessorEditor::~SpatialPannerAudioProcessorEditor() {}

void SpatialPannerAudioProcessorEditor::updateEnvButtons()
{
    for (int i = 0; i < 4; ++i) {
        const bool on = (activeEnvIdx == i+1);
        envBtns[i].setColour(juce::TextButton::buttonColourId,
                             on ? kEnvOn[i].darker(0.5f) : juce::Colour(0xff2e2e2e));
        envBtns[i].setColour(juce::TextButton::textColourOffId,
                             on ? kEnvOn[i].brighter(0.3f) : kDim);
    }
}

void SpatialPannerAudioProcessorEditor::applyReference(const juce::String& genre)
{
    canvas.setReference(genre.toUpperCase(), GenreData::forGenre(genre));
}

void SpatialPannerAudioProcessorEditor::timerCallback()
{
    trackList.refresh();

    float ml = 0.f, mr = 0.f, corr = 0.f, fWidth = 1.f;
    bool  found = false;
    for (auto& t : GlobalSpatialRegistry::get().snapshot())
        if (t.id == focusedTrackId) {
            if (t.meterL)      ml   = t.meterL->load();
            if (t.meterR)      mr   = t.meterR->load();
            if (t.correlation) corr = t.correlation->load();
            fWidth = t.width;
            found  = true;
            break;
        }

    // Width slider reflects focused track (without retriggering its callback)
    if (found && !widthSlider.isMouseButtonDown()
        && std::abs((float)widthSlider.getValue() - fWidth) > 0.001f)
        widthSlider.setValue(fWidth, juce::dontSendNotification);

    meterLDisplay = ml; meterRDisplay = mr;
    corrDisplay   = corr * 0.15f + corrDisplay * 0.85f;

    // Falling peak indicator (decays)
    if (ml > peakLDisplay) { peakLDisplay = ml; peakHoldFrames = 37; }
    if (mr > peakRDisplay) { peakRDisplay = mr; peakHoldFrames = 37; }
    if (peakHoldFrames > 0) --peakHoldFrames;
    else { peakLDisplay *= 0.93f; peakRDisplay *= 0.93f; }

    // Latched maxima (never decay until user resets)
    peakLatchL = juce::jmax(peakLatchL, ml);
    peakLatchR = juce::jmax(peakLatchR, mr);

    canvas.setFocusedId(focusedTrackId);

    const juce::String pn = processorRef.getTrackName();
    if (pn != nameEditor.getText() && !nameEditor.hasKeyboardFocus(false))
        nameEditor.setText(pn, false);

    uiPhase += 0.045f;
    if (uiPhase > juce::MathConstants<float>::twoPi) uiPhase -= juce::MathConstants<float>::twoPi;

    repaint();
}

void SpatialPannerAudioProcessorEditor::mouseDown(const juce::MouseEvent& e)
{
    // Unscale the click into base coordinates before hit-testing
    const juce::Point<int> base(
        juce::roundToInt(e.position.x / uiScale),
        juce::roundToInt(e.position.y / uiScale));
    if (peakResetBounds.contains(base))
    {
        peakLatchL = peakLatchR = 0.f;
        repaint();
    }
}

// ─── Layout (scaled) ───────────────────────────────────────────────────────────
void SpatialPannerAudioProcessorEditor::resized()
{
    uiScale = (float)getWidth() / (float)kW;
    auto S = [this](int v) { return juce::roundToInt(v * uiScale); };

    const int mainBottom = kMainY + kMainH;

    trackList.setBounds(0, S(kMainY), S(kListW), S(kMainH));
    canvas.setBounds(S(kCanvasX), S(kMainY), S(kCanvasW), S(kMainH));

    const int wY = mainBottom;
    widthSlider.setBounds(S(62), S(wY + 8), S(kW - 66), S(kWidthH - 16));

    const int bY = wY + kWidthH;
    nameEditor.setBounds(S(52), S(bY + 7), S(128), S(kBottomH - 14));

    const int btnW = 56, btnH = 24, btnY = (kTitleH - btnH) / 2;
    for (int i = 0; i < 4; ++i)
        envBtns[i].setBounds(S(kW - (4-i)*(btnW+4) - 6), S(btnY), S(btnW), S(btnH));

    const int envBlockX = kW - 4*(btnW+4) - 6;
    scopeBtn.setBounds(S(envBlockX - btnW - 12), S(btnY), S(btnW), S(btnH));
    refBtn.setBounds(S(envBlockX - 2*btnW - 18), S(btnY), S(btnW - 12), S(btnH));

    statusLabel.setBounds(S(6), S(kH - kStatusH), S(kW - kMeterW - 10), S(kStatusH));

    // Overlay covers the whole editor (actual pixels)
    genrePanel.setBounds(getLocalBounds());
}

// ─── Paint ────────────────────────────────────────────────────────────────────
void SpatialPannerAudioProcessorEditor::paint(juce::Graphics& g)
{
    // Scale all base-coordinate drawing to the current window size
    g.addTransform(juce::AffineTransform::scale(uiScale));

    // ── Body fill (S1 grey metal) ─────────────────────────────────────────────
    juce::ColourGradient bodyGrad(kPanel.brighter(0.05f), 0.f, 0.f,
                                   kPanel.darker(0.08f),  0.f, (float)kH, false);
    g.setGradientFill(bodyGrad);
    g.fillRect(0, 0, kW, kH);

    // ── Title bar (premium) ────────────────────────────────────────────────────
    {
        // Dark gradient base
        juce::ColourGradient tg(juce::Colour(0xff20242c), 0.f, 0.f,
                                 juce::Colour(0xff15181e), 0.f, (float)kTitleH, false);
        g.setGradientFill(tg);
        g.fillRect(0, 0, kW, kTitleH);

        // Animated cyan/violet glow sweep moving across the title
        const float sweepX = (0.5f + 0.5f * std::sin(uiPhase * 0.6f)) * kW;
        juce::ColourGradient sweep(juce::Colour(0x004466cc), sweepX - 160.f, 0.f,
                                    juce::Colour(0x004466cc), sweepX + 160.f, 0.f, false);
        sweep.addColour(0.5, juce::Colour(0xff5a9cff).withAlpha(0.16f));
        g.setGradientFill(sweep);
        g.fillRect(0, 0, kW, kTitleH);

        // Accent underline glow (animated brightness)
        const float pulse = 0.55f + 0.45f * std::sin(uiPhase);
        for (int k = 4; k >= 1; --k) {
            g.setColour(juce::Colour(0xff4a9cff).withAlpha(0.06f * k * pulse));
            g.drawLine(0.f, (float)kTitleH - (float)k, (float)kW, (float)kTitleH - (float)k, 1.f);
        }
        g.setColour(juce::Colour(0xff5aaaff).withAlpha(0.5f + 0.3f * pulse));
        g.drawLine(0.f, (float)kTitleH, (float)kW, (float)kTitleH, 1.5f);

        // Top highlight
        g.setColour(juce::Colour(0x22ffffff));
        g.drawLine(0.f, 0.5f, (float)kW, 0.5f, 1.f);

        // ── Logo mark: glowing diamond ─────────────────────────────────────────
        const float lx = 18.f, ly = kTitleH * 0.5f, lr = 7.f;
        for (int k = 4; k >= 1; --k) {
            g.setColour(juce::Colour(0xff5aaaff).withAlpha(0.10f * k * pulse));
            g.fillEllipse(lx - lr - k*2, ly - lr - k*2, (lr + k*2)*2.f, (lr + k*2)*2.f);
        }
        juce::Path dia; dia.addPolygon({lx, ly}, 4, lr, uiPhase * 0.5f);
        juce::ColourGradient dg(juce::Colour(0xffaad4ff), lx-lr*.4f, ly-lr*.4f,
                                 juce::Colour(0xff2a6cd0), lx+lr*.4f, ly+lr*.4f, false);
        g.setGradientFill(dg); g.fillPath(dia);
        g.setColour(juce::Colour(0xffd0e8ff)); g.fillEllipse(lx-1.5f, ly-1.5f, 3.f, 3.f);

        // Plugin name
        g.setFont(juce::Font(juce::FontOptions(14.f).withStyle("Bold")));
        g.setColour(juce::Colour(0xffe8eeff));
        g.drawText("SPATIAL PANNER", 34, 4, 220, kTitleH - 14, juce::Justification::centredLeft);
        g.setFont(juce::Font(juce::FontOptions(7.5f)));
        g.setColour(juce::Colour(0xff6a86b8));
        g.drawText("S P A T I A L   M I X E R   ·   v1.0", 35, kTitleH - 15, 220, 12,
                   juce::Justification::centredLeft);
    }

    // ── Canvas inset screen ───────────────────────────────────────────────────
    drawScreenInset(g, { kCanvasX - 2, kMainY - 2, kCanvasW + 4, kMainH + 4 });

    // ── Track list panel ──────────────────────────────────────────────────────
    {
        auto lr = juce::Rectangle<int>(0, kMainY, kListW, kMainH);
        fillPanel(g, lr);
        drawBevel(g, lr, false);
        // Separator right
        g.setColour(kDark);
        g.drawLine((float)kListW, (float)kMainY, (float)kListW, (float)(kMainY+kMainH), 1.f);
    }

    // ── Meter panel ───────────────────────────────────────────────────────────
    {
        auto mr = juce::Rectangle<int>(kMeterX, kMainY, kMeterW, kMainH);
        fillPanel(g, mr);
        drawBevel(g, mr, false);
        g.setColour(kDark);
        g.drawLine((float)kMeterX, (float)kMainY, (float)kMeterX, (float)(kMainY+kMainH), 1.f);
    }

    // ── Width row ─────────────────────────────────────────────────────────────
    {
        const int wY = kMainY + kMainH;
        fillPanel(g, { 0, wY, kW, kWidthH });
        g.setColour(kDark);
        g.drawLine(0.f, (float)wY, (float)kW, (float)wY, 1.5f);
        g.setColour(kHighlt);
        g.drawLine(0.f, (float)wY+1, (float)kW, (float)wY+1, 0.5f);
        g.setFont(juce::Font(juce::FontOptions(9.5f).withStyle("Bold")));
        g.setColour(kDim);
        g.drawText("WIDTH", 0, wY, 62, kWidthH, juce::Justification::centred);
    }

    // ── Bottom row (name + corr) ───────────────────────────────────────────────
    {
        const int bY = kMainY + kMainH + kWidthH;
        fillPanel(g, { 0, bY, kW, kBottomH });
        g.setColour(kDark);
        g.drawLine(0.f, (float)bY, (float)kW, (float)bY, 1.f);
        g.setFont(juce::Font(juce::FontOptions(9.f).withStyle("Bold")));
        g.setColour(kDim);
        g.drawText("NAME", 0, bY, 50, kBottomH, juce::Justification::centred);
        drawCorrelation(g, { 190, bY, kW - kMeterW - 190, kBottomH });
    }

    // ── Status bar ────────────────────────────────────────────────────────────
    {
        g.setColour(kPanel.darker(0.1f));
        g.fillRect(0, kH-kStatusH, kW, kStatusH);
        g.setColour(kDark);
        g.drawLine(0.f, (float)(kH-kStatusH), (float)kW, (float)(kH-kStatusH), 1.f);
    }

    // ── Meter (drawn last so it's on top of panel fill) ───────────────────────
    drawRightMeter(g, { kMeterX+1, kMainY+1, kMeterW-2, kMainH-2 });

    // ── Outer border bevel ────────────────────────────────────────────────────
    drawBevel(g, { 0, 0, kW, kH }, false);
}

// ─── Right meter ─────────────────────────────────────────────────────────────
void SpatialPannerAudioProcessorEditor::drawRightMeter(juce::Graphics& g,
                                                        juce::Rectangle<int> b)
{
    const float bx = b.getX(), by = b.getY(), bw = b.getWidth(), bh = b.getHeight();

    // Header
    g.setFont(juce::Font(juce::FontOptions(8.f).withStyle("Bold")));
    g.setColour(kDim);
    g.drawText("OUTPUT", (int)bx, (int)by+2, (int)bw, 12, juce::Justification::centred);

    // ── Latched peak readout (click to reset) ─────────────────────────────────
    const float latch = juce::jmax(peakLatchL, peakLatchR);
    const float latchDb = latch > 0.f ? 20.f * std::log10(juce::jmax(1e-6f, latch)) : -99.f;
    const juce::String latchStr = latchDb < -60.f ? "-Inf" : (juce::String(latchDb, 1));

    peakResetBounds = { (int)bx + 6, (int)by + 14, (int)bw - 12, 16 };
    g.setColour(juce::Colour(0xff141414));
    g.fillRect(peakResetBounds);
    g.setColour(latch > 0.92f ? juce::Colour(0xffee2222)
              : latch > 0.75f ? juce::Colour(0xffeebb11)
                              : juce::Colour(0xff66bb66));
    g.setFont(juce::Font(juce::FontOptions(9.5f).withStyle("Bold")));
    g.drawText(latchStr, peakResetBounds, juce::Justification::centred);
    g.setColour(kBorder);
    g.drawRect(peakResetBounds, 1);

    const float scaleTop = by + 36.f;
    const float scaleBot = by + bh - 32.f;
    const float scaleH   = scaleBot - scaleTop;

    auto normToY = [&](float n) {
        return scaleBot - juce::jlimit(0.f, 1.f, n) * scaleH;
    };

    // Scale marks (S1 style: tick marks + labels)
    constexpr float kMarks[] = { 0.f, -3.f, -6.f, -12.f, -18.f, -24.f, -30.f };
    g.setFont(juce::Font(juce::FontOptions(7.5f)));
    for (float db : kMarks) {
        const float y = normToY(juce::Decibels::decibelsToGain(db));
        g.setColour(db == 0.f ? juce::Colour(0x88ee3333) : kDim.withAlpha(0.4f));
        g.drawLine(bx+3.f, y, bx+bw-3.f, y, 0.5f);
        g.setColour(kDim.withAlpha(0.55f));
        g.drawText(db==0.f ? "0" : juce::String((int)db),
                   (int)bx+1, (int)y-5, (int)bw-2, 11, juce::Justification::centred);
    }

    // Meter bars
    const float mW = 12.f;
    const float lx = bx + bw*0.5f - mW - 3.f;
    const float rx = bx + bw*0.5f + 3.f;

    // LED-segment meter with rounded inset trough and glowing active segments
    auto drawBar = [&](float x, float norm, float peak) {
        // Inset trough
        g.setColour(juce::Colour(0xff0a0a0c));
        g.fillRoundedRectangle(x - 1.f, scaleTop - 1.f, mW + 2.f, scaleH + 2.f, 2.5f);
        g.setColour(juce::Colour(0xff000000));
        g.fillRoundedRectangle(x, scaleTop, mW, scaleH, 2.f);

        const int   segs   = 26;
        const float segGap = 1.4f;
        const float segH   = (scaleH - segGap * (segs - 1)) / segs;
        const float litN   = juce::jlimit(0.f, 1.f, norm);

        for (int s = 0; s < segs; ++s) {
            const float frac = (float)(s + 1) / segs;        // 0..1 bottom→top
            const float segY = scaleBot - (s + 1) * segH - s * segGap;
            juce::Colour c = frac > 0.90f ? juce::Colour(0xffff2a2a)
                           : frac > 0.72f ? juce::Colour(0xffffc020)
                                          : juce::Colour(0xff2ce05a);
            if (frac <= litN) {
                // glow
                g.setColour(c.withAlpha(0.25f));
                g.fillRoundedRectangle(x - 1.f, segY - 0.5f, mW + 2.f, segH + 1.f, 1.5f);
                // lit segment
                g.setColour(c);
                g.fillRoundedRectangle(x, segY, mW, segH, 1.f);
                // top sheen
                g.setColour(juce::Colours::white.withAlpha(0.15f));
                g.fillRect(x, segY, mW, segH * 0.4f);
            } else {
                // dim unlit segment
                g.setColour(c.withAlpha(0.08f));
                g.fillRoundedRectangle(x, segY, mW, segH, 1.f);
            }
        }

        // Peak hold marker — bright line + glow
        if (peak > 0.001f) {
            const float py = normToY(juce::jlimit(0.f, 1.f, peak));
            g.setColour(juce::Colours::white.withAlpha(0.4f));
            g.fillRect(x - 1.f, py - 1.5f, mW + 2.f, 3.f);
            g.setColour(juce::Colours::white);
            g.fillRect(x, py - 1.f, mW, 1.8f);
        }
    };

    drawBar(lx, meterLDisplay, peakLDisplay);
    drawBar(rx, meterRDisplay, peakRDisplay);

    // L / R labels
    g.setFont(juce::Font(juce::FontOptions(7.5f).withStyle("Bold")));
    g.setColour(kDim);
    g.drawText("L", (int)lx, (int)scaleBot+3, (int)mW, 11, juce::Justification::centred);
    g.drawText("R", (int)rx, (int)scaleBot+3, (int)mW, 11, juce::Justification::centred);

    // dB numeric readout
    const float db = meterLDisplay > 0.f
        ? 20.f * std::log10(juce::jmax(1e-6f, meterLDisplay)) : -99.f;
    const juce::String dbStr = db < -60.f ? "-Inf" : (juce::String(db,1)+"dB");
    g.setFont(juce::Font(juce::FontOptions(8.5f)));
    g.setColour(meterCol(meterLDisplay));
    g.drawText(dbStr, (int)bx, (int)scaleBot+15, (int)bw, 12, juce::Justification::centred);
}

// ─── Correlation ─────────────────────────────────────────────────────────────
void SpatialPannerAudioProcessorEditor::drawCorrelation(juce::Graphics& g,
                                                         juce::Rectangle<int> b)
{
    const float bx=b.getX(), by=b.getY(), bw=b.getWidth(), bh=b.getHeight();

    g.setFont(juce::Font(juce::FontOptions(8.f).withStyle("Bold")));
    g.setColour(kDim);
    g.drawText("CORR", (int)bx, (int)by, 32, (int)bh, juce::Justification::centredLeft);

    const float barX=bx+34.f, barW=bw-58.f, barY=by+7.f, barH=bh-14.f;

    // Track background (inset look)
    g.setColour(juce::Colour(0xff141414));
    g.fillRoundedRectangle(barX, barY, barW, barH, 2.f);
    g.setColour(kBorder);
    g.drawRoundedRectangle(barX, barY, barW, barH, 2.f, 0.5f);
    g.setColour(kHighlt.withAlpha(0.4f));
    g.drawLine(barX, barY+barH, barX+barW, barY+barH, 0.5f);

    // Centre mark
    const float cx = barX + barW*0.5f;
    g.setColour(kDim.withAlpha(0.35f));
    g.drawLine(cx, barY+1.f, cx, barY+barH-1.f, 0.5f);

    // Fill
    const float corr = juce::jlimit(-1.f, 1.f, corrDisplay);
    const float fillX = corr>=0.f ? cx : barX+(corr+1.f)*0.5f*barW;
    const float fillW = std::abs(corr)*barW*0.5f;
    juce::Colour bc = corr>=0.f ? juce::Colour(0xff22bb44) : juce::Colour(0xffcc3333);
    g.setColour(bc.withAlpha(0.75f));
    g.fillRoundedRectangle(fillX, barY+1.f, fillW, barH-2.f, 1.5f);

    g.setFont(juce::Font(juce::FontOptions(9.f)));
    g.setColour(bc.brighter(0.3f));
    g.drawText(juce::String(corr,2), (int)(barX+barW+4), (int)by, 22, (int)bh,
               juce::Justification::centredLeft);
}
