#include "WavetableBank.h"

void WavetableBank::prepare(double sr, int /*samplesPerBlock*/)
{
    sampleRate = sr;
}

void WavetableBank::reset()
{
    clear();
}

void WavetableBank::loadFromBuffer(const juce::String& name,
                                   const juce::AudioBuffer<float>& /*buffer*/,
                                   double /*bufferSampleRate*/)
{
    setNames.push_back(name);
}

juce::String WavetableBank::getSetName(int index) const
{
    if (index >= 0 && index < static_cast<int>(setNames.size()))
        return setNames[static_cast<size_t>(index)];
    return {};
}

void WavetableBank::clear()
{
    setNames.clear();
}
