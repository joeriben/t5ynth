#pragma once
#include <JuceHeader.h>

/**
 * Preset serialization and deserialization.
 *
 * Format: JSON containing parameter values, axis definitions,
 * drift LFO config, and optionally embedded wavetable data.
 */
class PresetFormat
{
public:
    PresetFormat() = default;

    /** Save current state to a preset file. */
    bool saveToFile(const juce::File& file,
                    juce::AudioProcessorValueTreeState& parameters);

    /** Load a preset from file and apply to parameters. */
    bool loadFromFile(const juce::File& file,
                      juce::AudioProcessorValueTreeState& parameters);

    /** Get the preset file extension. */
    static juce::String getFileExtension() { return ".t5p"; }

    /** Get default preset directory. */
    static juce::File getPresetsDirectory();

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetFormat)
};
