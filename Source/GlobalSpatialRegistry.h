#pragma once
#include <JuceHeader.h>
#include <map>
#include <memory>
#include <atomic>

static const juce::Colour kTrackPalette[] = {
    juce::Colour(0xffff6b6b), juce::Colour(0xff4ecdc4),
    juce::Colour(0xffffd93d), juce::Colour(0xff6bcb77),
    juce::Colour(0xff4d96ff), juce::Colour(0xffff9f43),
    juce::Colour(0xffc77dff), juce::Colour(0xffff78a9),
    juce::Colour(0xff50e3c2), juce::Colour(0xffffb347),
};
static constexpr int kPaletteSize = 10;

struct TrackEntry
{
    int          id    = -1;
    float        posX  =  0.f;
    float        posY  =  0.f;
    float        width =  1.f;
    juce::Colour colour;
    juce::String label;

    std::shared_ptr<std::atomic<bool>> alive;
    juce::RangedAudioParameter* panParam   = nullptr;
    juce::RangedAudioParameter* depthParam = nullptr;
    juce::RangedAudioParameter* widthParam = nullptr;

    // Written on audio thread, read on message thread
    std::atomic<float>* meterL      = nullptr;
    std::atomic<float>* meterR      = nullptr;
    std::atomic<float>* correlation = nullptr;

    // Vectorscope buffers (read-only on message thread)
    const float*           scopeL   = nullptr;
    const float*           scopeR   = nullptr;
    std::atomic<uint32_t>* scopePos = nullptr;
    int                    scopeSize = 0;
};

class GlobalSpatialRegistry
{
public:
    static GlobalSpatialRegistry& get();

    int  add(std::shared_ptr<std::atomic<bool>> alive,
             juce::RangedAudioParameter* pan,
             juce::RangedAudioParameter* depth,
             juce::RangedAudioParameter* width,
             std::atomic<float>* mL,
             std::atomic<float>* mR,
             std::atomic<float>* corr,
             const float* scopeL,
             const float* scopeR,
             std::atomic<uint32_t>* scopePos,
             int scopeSize);
    void remove(int id);

    void sync(int id, float x, float y, float w);
    void setName(int id, const juce::String& name);
    void setPosition(int id, float x, float y);
    void setWidth(int id, float w);

    std::vector<TrackEntry> snapshot() const;
    int  addListener(std::function<void()> fn);
    void removeListener(int lid);

private:
    GlobalSpatialRegistry() = default;

    mutable juce::CriticalSection        cs;
    std::map<int, TrackEntry>            entries;
    std::atomic<int>                     nextId { 0 };
    std::map<int, std::function<void()>> listeners;
    std::atomic<int>                     nextListenerId { 0 };

    void fire();
};
