#pragma once
#include <JuceHeader.h>
#include <memory>
#include <atomic>
#include "GlobalSpatialRegistry.h"

class SpatialPannerAudioProcessor
    : public juce::AudioProcessor,
      public juce::AudioProcessorValueTreeState::Listener
{
public:
    SpatialPannerAudioProcessor();
    ~SpatialPannerAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi()  const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;
    void parameterChanged(const juce::String&, float) override;
    void updateTrackProperties(const TrackProperties& props) override;

    void setTrackName(const juce::String& name);
    juce::String getTrackName() const { return trackName; }

    // Called from editor preset buttons — queues environment change
    void requestEnvironment(int mode);  // 0=Off 1=Studio 2=Club 3=Car 4=Phone
    int  getEnvironmentMode() const { return envModeRequested.load(); }

    juce::AudioProcessorValueTreeState apvts;
    int registryId = -1;

    std::atomic<float> meterL      { 0.f };
    std::atomic<float> meterR      { 0.f };
    std::atomic<float> correlation { 0.f };

    // Vectorscope ring buffer (post-processing L/R samples)
    static constexpr int kScopeSize = 2048;
    static constexpr int kScopeMask = kScopeSize - 1;
    float scopeL[kScopeSize] {};
    float scopeR[kScopeSize] {};
    std::atomic<uint32_t> scopePos { 0 };

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameters();

    // Environment DSP
    std::atomic<int>  envModeRequested { 0 };
    int               envModeActive    { -1 };  // -1 forces init on first block

    juce::Reverb      envReverb;
    juce::IIRFilter   envFilterL[3], envFilterR[3];
    bool              envFilterOn[3]  { false, false, false };

    void configureEnvironment(int mode);
    void applyEnvironment(float* L, float* R, int n);

    std::shared_ptr<std::atomic<bool>> aliveFlag;
    juce::String trackName;
    double currentSampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpatialPannerAudioProcessor)
};
