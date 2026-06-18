#include "GlobalSpatialRegistry.h"

GlobalSpatialRegistry& GlobalSpatialRegistry::get()
{
    static GlobalSpatialRegistry inst;
    return inst;
}

int GlobalSpatialRegistry::add(std::shared_ptr<std::atomic<bool>> alive,
                                juce::RangedAudioParameter* pan,
                                juce::RangedAudioParameter* depth,
                                juce::RangedAudioParameter* width,
                                std::atomic<float>* mL,
                                std::atomic<float>* mR,
                                std::atomic<float>* corr,
                                const float* scopeL,
                                const float* scopeR,
                                std::atomic<uint32_t>* scopePos,
                                int scopeSize)
{
    const int id = nextId++;
    TrackEntry e;
    e.id          = id;
    e.alive       = alive;
    e.panParam    = pan;
    e.depthParam  = depth;
    e.widthParam  = width;
    e.meterL      = mL;
    e.meterR      = mR;
    e.correlation = corr;
    e.scopeL      = scopeL;
    e.scopeR      = scopeR;
    e.scopePos    = scopePos;
    e.scopeSize   = scopeSize;
    e.colour      = kTrackPalette[id % kPaletteSize];
    e.label       = "Tr " + juce::String(id + 1);
    { juce::ScopedLock sl(cs); entries[id] = std::move(e); }
    fire();
    return id;
}

void GlobalSpatialRegistry::remove(int id)
{
    { juce::ScopedLock sl(cs); entries.erase(id); }
    fire();
}

void GlobalSpatialRegistry::sync(int id, float x, float y, float w)
{
    juce::ScopedLock sl(cs);
    auto it = entries.find(id);
    if (it == entries.end()) return;
    it->second.posX = x; it->second.posY = y; it->second.width = w;
    // No fire() here - audio thread calls this too often; canvas timer handles refresh
}

void GlobalSpatialRegistry::setName(int id, const juce::String& name)
{
    { juce::ScopedLock sl(cs);
      auto it = entries.find(id);
      if (it == entries.end()) return;
      it->second.label = name.isEmpty() ? ("Tr " + juce::String(id + 1)) : name; }
    fire();
}

void GlobalSpatialRegistry::setPosition(int id, float x, float y)
{
    juce::RangedAudioParameter* pp = nullptr, *dp = nullptr;
    std::shared_ptr<std::atomic<bool>> alive;
    { juce::ScopedLock sl(cs);
      auto it = entries.find(id);
      if (it == entries.end()) return;
      pp = it->second.panParam; dp = it->second.depthParam; alive = it->second.alive; }
    if (alive && alive->load() && pp && dp) {
        pp->setValueNotifyingHost(pp->convertTo0to1(x));
        dp->setValueNotifyingHost(dp->convertTo0to1(y));
    }
}

void GlobalSpatialRegistry::setWidth(int id, float w)
{
    juce::RangedAudioParameter* wp = nullptr;
    std::shared_ptr<std::atomic<bool>> alive;
    { juce::ScopedLock sl(cs);
      auto it = entries.find(id);
      if (it == entries.end()) return;
      wp = it->second.widthParam; alive = it->second.alive; }
    if (alive && alive->load() && wp)
        wp->setValueNotifyingHost(wp->convertTo0to1(w));
}

std::vector<TrackEntry> GlobalSpatialRegistry::snapshot() const
{
    juce::ScopedLock sl(cs);
    std::vector<TrackEntry> out;
    out.reserve(entries.size());
    for (auto& [id, e] : entries) out.push_back(e);
    return out;
}

int GlobalSpatialRegistry::addListener(std::function<void()> fn)
{
    const int lid = nextListenerId++;
    juce::ScopedLock sl(cs);
    listeners[lid] = std::move(fn);
    return lid;
}

void GlobalSpatialRegistry::removeListener(int lid)
{
    juce::ScopedLock sl(cs);
    listeners.erase(lid);
}

void GlobalSpatialRegistry::fire()
{
    // Dispatch a single async tick. The listeners map is re-read at execution time
    // (under the lock) rather than captured, so a listener that deregistered between
    // dispatch and execution is never invoked with a dangling `this`. The registry
    // is a process-lifetime singleton, so capturing it by reference is safe.
    juce::MessageManager::callAsync([this]() {
        std::vector<std::function<void()>> snapshot;
        { juce::ScopedLock sl(cs);
          snapshot.reserve(listeners.size());
          for (auto& [id, fn] : listeners) snapshot.push_back(fn); }
        for (auto& fn : snapshot) fn();
    });
}
