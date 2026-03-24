#pragma once
#include <JuceHeader.h>

/**
 * Root GUI panel containing all sub-panels.
 *
 * Manages the layout of PromptPanel, SynthPanel, EffectsPanel,
 * SequencerPanel, AxesPanel, etc.
 */
class MainPanel : public juce::Component
{
public:
    MainPanel();
    ~MainPanel() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainPanel)
};
