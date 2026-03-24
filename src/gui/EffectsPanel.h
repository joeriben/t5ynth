#pragma once
#include <JuceHeader.h>

/**
 * Panel for effects controls: filter, delay, reverb.
 */
class EffectsPanel : public juce::Component
{
public:
    EffectsPanel();
    ~EffectsPanel() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EffectsPanel)
};
