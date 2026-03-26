#pragma once
#include <JuceHeader.h>
#include "GuiHelpers.h"

/**
 * FX column (20%): Delay + Reverb.
 * All controls are horizontal linear slider rows.
 * Limiter is internal only — no GUI controls.
 */
class FxPanel : public juce::Component
{
public:
    explicit FxPanel(juce::AudioProcessorValueTreeState& apvts);
    ~FxPanel() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    float fs() const;

    // Delay
    juce::ToggleButton delayToggle { "Delay" };
    std::unique_ptr<SliderRow> delayTimeRow, delayFbRow, delayDampRow, delayMixRow;

    // Reverb
    juce::ToggleButton reverbToggle { "Reverb" };
    juce::ComboBox reverbIrBox;
    std::unique_ptr<SliderRow> reverbMixRow;

    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using CA = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<SA> delayTimeA, delayFbA, delayDampA, delayMixA;
    std::unique_ptr<SA> reverbMixA;
    std::unique_ptr<CA> reverbIrA;
    std::unique_ptr<BA> delayEnableA, reverbEnableA;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FxPanel)
};
