#pragma once
#include <JuceHeader.h>

/**
 * Custom LookAndFeel for T5ynth.
 *
 * Dark theme with #0a0a0a background, consistent with the project's
 * visual identity.
 */
class T5ynthLookAndFeel : public juce::LookAndFeel_V4
{
public:
    T5ynthLookAndFeel();
    ~T5ynthLookAndFeel() override = default;

    // Colour IDs
    static constexpr juce::uint32 backgroundColourId = 0x1000000;
    static constexpr juce::uint32 accentColourId     = 0x1000001;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(T5ynthLookAndFeel)
};
