#pragma once
#include <JuceHeader.h>
#include <array>
#include "GuiHelpers.h"

class T5ynthProcessor;

class SequencerPanel : public juce::Component
{
public:
    explicit SequencerPanel(T5ynthProcessor& processor);
    ~SequencerPanel() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;

private:
    T5ynthProcessor& processorRef;

    juce::TextButton playButton { ">" };
    juce::TextButton stopButton { "||" };
    juce::ComboBox modeBox;

    // Linear slider rows instead of rotary knobs
    std::unique_ptr<SliderRow> bpmRow;
    std::unique_ptr<SliderRow> octRow;

    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using CA = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::unique_ptr<SA> bpmAttach, octAttach;
    std::unique_ptr<CA> modeAttach;

    static constexpr int numVisibleSteps = 16;
    std::array<bool, 64> stepStates {};

    juce::Rectangle<int> getStepBounds(int step) const;
    juce::Rectangle<int> gridArea;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SequencerPanel)
};
