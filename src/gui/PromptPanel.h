#pragma once
#include <JuceHeader.h>

/**
 * Panel for entering text prompts and triggering generation.
 */
class PromptPanel : public juce::Component
{
public:
    PromptPanel();
    ~PromptPanel() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PromptPanel)
};
