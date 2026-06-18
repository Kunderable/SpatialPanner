#include "PluginProcessor.h"
#include "PluginEditor.h"

SpatialPannerAudioProcessor::SpatialPannerAudioProcessor()
    : AudioProcessor(BusesProperties()
          .withInput ("Input",  juce::AudioChannelSet::stereo(), true)
          .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameters())
{
    aliveFlag  = std::make_shared<std::atomic<bool>>(true);
    registryId = GlobalSpatialRegistry::get().add(
        aliveFlag,
        dynamic_cast<juce::RangedAudioParameter*>(apvts.getParameter("posX")),
        dynamic_cast<juce::RangedAudioParameter*>(apvts.getParameter("posY")),
        dynamic_cast<juce::RangedAudioParameter*>(apvts.getParameter("width")),
        &meterL, &meterR, &correlation,
        scopeL, scopeR, &scopePos, kScopeSize);

    apvts.addParameterListener("posX",  this);
    apvts.addParameterListener("posY",  this);
    apvts.addParameterListener("width", this);
}

SpatialPannerAudioProcessor::~SpatialPannerAudioProcessor()
{
    apvts.removeParameterListener("posX",  this);
    apvts.removeParameterListener("posY",  this);
    apvts.removeParameterListener("width", this);
    *aliveFlag = false;
    GlobalSpatialRegistry::get().remove(registryId);
}

juce::AudioProcessorValueTreeState::ParameterLayout
SpatialPannerAudioProcessor::createParameters()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"posX",  1}, "Pan",
        juce::NormalisableRange<float>(-1.f, 1.f, 0.001f), 0.f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"posY",  1}, "Depth",
        juce::NormalisableRange<float>(0.f, 1.f, 0.001f), 0.f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"width", 1}, "Stereo Width",
        juce::NormalisableRange<float>(0.f, 4.f, 0.001f), 1.f));
    return { p.begin(), p.end() };
}

void SpatialPannerAudioProcessor::parameterChanged(const juce::String&, float)
{
    GlobalSpatialRegistry::get().sync(registryId,
        apvts.getRawParameterValue("posX")->load(),
        apvts.getRawParameterValue("posY")->load(),
        apvts.getRawParameterValue("width")->load());
}

void SpatialPannerAudioProcessor::updateTrackProperties(const TrackProperties& props)
{
    if (props.name.isNotEmpty())
        juce::MessageManager::callAsync([this, name = props.name]()
        {
            trackName = name;
            GlobalSpatialRegistry::get().setName(registryId, name);
            if (auto* ed = getActiveEditor()) ed->repaint();
        });
}

void SpatialPannerAudioProcessor::setTrackName(const juce::String& name)
{
    trackName = name;
    GlobalSpatialRegistry::get().setName(registryId, name);
}

void SpatialPannerAudioProcessor::requestEnvironment(int mode)
{
    envModeRequested.store(mode);
}

// Environment DSP

void SpatialPannerAudioProcessor::configureEnvironment(int mode)
{
    const double sr = currentSampleRate;
    auto dB = [](float db) { return std::pow(10.f, db / 20.f); };

    for (auto& f : envFilterOn) f = false;
    envReverb.reset();

    juce::Reverb::Parameters rp;
    rp.freezeMode = 0.f;
    rp.width      = 1.f;

    switch (mode)
    {
    case 1: // Studio - light room, flat EQ
        rp.roomSize = 0.30f; rp.damping = 0.60f;
        rp.wetLevel = 0.07f; rp.dryLevel = 1.0f;
        break;

    case 2: // Club - large room, bass boost, mid scoop, air
        rp.roomSize = 0.85f; rp.damping = 0.22f;
        rp.wetLevel = 0.38f; rp.dryLevel = 1.0f;
        // Low shelf +5dB @ 90Hz
        envFilterL[0].setCoefficients(juce::IIRCoefficients::makeLowShelf (sr, 90.0, 0.707, dB(5.f)));
        envFilterR[0].setCoefficients(juce::IIRCoefficients::makeLowShelf (sr, 90.0, 0.707, dB(5.f)));
        envFilterOn[0] = true;
        // Peak -2.5dB @ 2kHz (mid scoop)
        envFilterL[1].setCoefficients(juce::IIRCoefficients::makePeakFilter(sr, 2000.0, 1.4, dB(-2.5f)));
        envFilterR[1].setCoefficients(juce::IIRCoefficients::makePeakFilter(sr, 2000.0, 1.4, dB(-2.5f)));
        envFilterOn[1] = true;
        // High shelf +2dB @ 12kHz (air)
        envFilterL[2].setCoefficients(juce::IIRCoefficients::makeHighShelf(sr, 12000.0, 0.707, dB(2.f)));
        envFilterR[2].setCoefficients(juce::IIRCoefficients::makeHighShelf(sr, 12000.0, 0.707, dB(2.f)));
        envFilterOn[2] = true;
        break;

    case 3: // Car - small boxy room, bass resonance, rolled-off highs
        rp.roomSize = 0.22f; rp.damping = 0.82f;
        rp.wetLevel = 0.12f; rp.dryLevel = 1.0f;
        // Low shelf +3dB @ 90Hz
        envFilterL[0].setCoefficients(juce::IIRCoefficients::makeLowShelf (sr, 90.0, 0.707, dB(3.f)));
        envFilterR[0].setCoefficients(juce::IIRCoefficients::makeLowShelf (sr, 90.0, 0.707, dB(3.f)));
        envFilterOn[0] = true;
        // Peak +3dB @ 200Hz (boxy)
        envFilterL[1].setCoefficients(juce::IIRCoefficients::makePeakFilter(sr, 200.0, 2.5, dB(3.f)));
        envFilterR[1].setCoefficients(juce::IIRCoefficients::makePeakFilter(sr, 200.0, 2.5, dB(3.f)));
        envFilterOn[1] = true;
        // High shelf -5dB @ 3kHz (muffled interior)
        envFilterL[2].setCoefficients(juce::IIRCoefficients::makeHighShelf(sr, 3000.0, 0.707, dB(-5.f)));
        envFilterR[2].setCoefficients(juce::IIRCoefficients::makeHighShelf(sr, 3000.0, 0.707, dB(-5.f)));
        envFilterOn[2] = true;
        break;

    case 4: // Phone - bandpass 300–3400Hz, tiny reverb
        rp.roomSize = 0.10f; rp.damping = 0.90f;
        rp.wetLevel = 0.04f; rp.dryLevel = 1.0f;
        // High-pass @ 300Hz
        envFilterL[0].setCoefficients(juce::IIRCoefficients::makeHighPass(sr, 300.0, 0.9));
        envFilterR[0].setCoefficients(juce::IIRCoefficients::makeHighPass(sr, 300.0, 0.9));
        envFilterOn[0] = true;
        // Low-pass @ 3400Hz
        envFilterL[1].setCoefficients(juce::IIRCoefficients::makeLowPass (sr, 3400.0, 0.9));
        envFilterR[1].setCoefficients(juce::IIRCoefficients::makeLowPass (sr, 3400.0, 0.9));
        envFilterOn[1] = true;
        break;

    default: // Off
        rp.roomSize = 0.f; rp.damping = 0.f;
        rp.wetLevel = 0.f; rp.dryLevel = 1.f;
        break;
    }

    envReverb.setParameters(rp);
}

void SpatialPannerAudioProcessor::applyEnvironment(float* L, float* R, int n)
{
    // Reverb
    envReverb.processStereo(L, R, n);

    // EQ bands
    for (int b = 0; b < 3; ++b)
    {
        if (!envFilterOn[b]) continue;
        for (int i = 0; i < n; ++i)
        {
            L[i] = envFilterL[b].processSingleSampleRaw(L[i]);
            R[i] = envFilterR[b].processSingleSampleRaw(R[i]);
        }
    }
}

// Audio

void SpatialPannerAudioProcessor::prepareToPlay(double sr, int)
{
    currentSampleRate = sr;
    meterL.store(0.f); meterR.store(0.f); correlation.store(0.f);
    envReverb.setSampleRate(sr);
    envReverb.reset();
    for (auto& f : envFilterL) f.reset();
    for (auto& f : envFilterR) f.reset();
    envModeActive = -1; // force reconfigure
}

void SpatialPannerAudioProcessor::releaseResources() {}

bool SpatialPannerAudioProcessor::isBusesLayoutSupported(const BusesLayout& l) const
{
    return l.getMainInputChannelSet()  == juce::AudioChannelSet::stereo()
        && l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void SpatialPannerAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                               juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    if (buffer.getNumChannels() < 2) return;

    // Reconfigure environment if changed
    const int envMode = envModeRequested.load();
    if (envMode != envModeActive)
    {
        configureEnvironment(envMode);
        envModeActive = envMode;
    }

    const float panX  = apvts.getRawParameterValue("posX")->load();
    const float depth = apvts.getRawParameterValue("posY")->load();
    const float width = apvts.getRawParameterValue("width")->load();
    const int   n     = buffer.getNumSamples();
    float*      L     = buffer.getWritePointer(0);
    float*      R     = buffer.getWritePointer(1);

    // Spatial processing
    const float gain  = juce::Decibels::decibelsToGain(-depth * 15.f);
    const float angle = ((panX + 1.f) * 0.5f) * juce::MathConstants<float>::halfPi;
    const float panL  = std::cos(angle);
    const float panR  = std::sin(angle);

    float peakL = 0.f, peakR = 0.f;
    float sumLR = 0.f, sumL2 = 0.f, sumR2 = 0.f;

    for (int i = 0; i < n; ++i)
    {
        const float mid  = (L[i] + R[i]) * 0.5f;
        const float side = (L[i] - R[i]) * 0.5f;
        L[i] = (mid + side * width) * panL * gain;
        R[i] = (mid - side * width) * panR * gain;
        peakL = std::max(peakL, std::abs(L[i]));
        peakR = std::max(peakR, std::abs(R[i]));
        sumLR += L[i] * R[i];
        sumL2 += L[i] * L[i];
        sumR2 += R[i] * R[i];
    }

    // Environment DSP (after spatial)
    if (envMode != 0)
        applyEnvironment(L, R, n);

    // Feed vectorscope ring buffer
    {
        uint32_t pos = scopePos.load(std::memory_order_relaxed);
        for (int i = 0; i < n; ++i)
        {
            scopeL[pos & kScopeMask] = L[i];
            scopeR[pos & kScopeMask] = R[i];
            ++pos;
        }
        scopePos.store(pos, std::memory_order_release);
    }

    // Meters + correlation
    const float decay = std::pow(0.001f, (float)n / (float)currentSampleRate);
    meterL.store(std::max(peakL, meterL.load(std::memory_order_relaxed) * decay));
    meterR.store(std::max(peakR, meterR.load(std::memory_order_relaxed) * decay));

    const float denom = std::sqrt(sumL2 * sumR2);
    const float corr  = denom > 1e-10f ? juce::jlimit(-1.f, 1.f, sumLR / denom) : 0.f;
    correlation.store(correlation.load(std::memory_order_relaxed) * 0.88f + corr * 0.12f);
}

// State

void SpatialPannerAudioProcessor::getStateInformation(juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    state.setProperty("trackName", trackName, nullptr);
    state.setProperty("envMode",   envModeRequested.load(), nullptr);
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, dest);
}

void SpatialPannerAudioProcessor::setStateInformation(const void* data, int size)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, size));
    if (xml && xml->hasTagName(apvts.state.getType()))
    {
        auto tree = juce::ValueTree::fromXml(*xml);
        apvts.replaceState(tree);
        trackName = tree.getProperty("trackName", "").toString();
        GlobalSpatialRegistry::get().setName(registryId, trackName);
        envModeRequested.store((int)tree.getProperty("envMode", 0));
    }
}

juce::AudioProcessorEditor* SpatialPannerAudioProcessor::createEditor()
{
    return new SpatialPannerAudioProcessorEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SpatialPannerAudioProcessor();
}
