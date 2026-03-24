#pragma once
#include <JuceHeader.h>

/**
 * Panel for synth controls: oscillator, envelope, scan position.
 */
class SynthPanel : public juce::Component
{
public:
    SynthPanel();
    ~SynthPanel() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SynthPanel)
};
