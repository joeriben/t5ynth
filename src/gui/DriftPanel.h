#pragma once
#include <JuceHeader.h>

/**
 * Panel for configuring the DriftLFO system.
 *
 * Shows 3 internal LFOs with rate, depth, and target selectors.
 */
class DriftPanel : public juce::Component
{
public:
    DriftPanel();
    ~DriftPanel() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DriftPanel)
};
