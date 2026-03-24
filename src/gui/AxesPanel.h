#pragma once
#include <JuceHeader.h>

/**
 * Panel for semantic axes sliders.
 */
class AxesPanel : public juce::Component
{
public:
    AxesPanel();
    ~AxesPanel() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AxesPanel)
};
