#pragma once
#include <JuceHeader.h>

/**
 * Panel for loading and saving presets.
 */
class PresetPanel : public juce::Component
{
public:
    PresetPanel();
    ~PresetPanel() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetPanel)
};
