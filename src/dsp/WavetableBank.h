#pragma once
#include <JuceHeader.h>
#include <vector>

/**
 * Bank of wavetable frames loaded from various sources.
 *
 * Manages a collection of named wavetable sets that the oscillator can
 * scan through. Handles loading from generated audio, files, or factory presets.
 */
class WavetableBank
{
public:
    WavetableBank() = default;

    void prepare(double sampleRate, int samplesPerBlock);
    void reset();

    /** Load frames from an audio buffer into a named slot. */
    void loadFromBuffer(const juce::String& name,
                        const juce::AudioBuffer<float>& buffer,
                        double bufferSampleRate);

    /** Get the number of loaded wavetable sets. */
    int getNumSets() const { return static_cast<int>(setNames.size()); }

    /** Get a set name by index. */
    juce::String getSetName(int index) const;

    /** Clear all loaded sets. */
    void clear();

private:
    double sampleRate = 44100.0;
    std::vector<juce::String> setNames;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WavetableBank)
};
