#pragma once
#include <JuceHeader.h>

/**
 * Panel for step sequencer and arpeggiator controls.
 */
class SequencerPanel : public juce::Component
{
public:
    SequencerPanel();
    ~SequencerPanel() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SequencerPanel)
};
