#pragma once
#include <JuceHeader.h>

/**
 * FILTER / FX column: filter, delay, reverb, limiter.
 */
class EffectsPanel : public juce::Component
{
public:
    explicit EffectsPanel(juce::AudioProcessorValueTreeState& apvts);
    ~EffectsPanel() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    // Filter
    juce::Slider cutoffKnob, resoKnob;
    juce::Label cutoffLabel, resoLabel;
    juce::ComboBox filterTypeBox;

    // Delay
    juce::Slider delayTimeKnob, delayFbKnob, delayMixKnob;
    juce::Label delayTimeLabel, delayFbLabel, delayMixLabel;

    // Reverb
    juce::Slider reverbMixKnob;
    juce::Label reverbMixLabel;
    juce::ComboBox reverbIrBox;

    // Limiter
    juce::Slider limThreshKnob, limReleaseKnob;
    juce::Label limThreshLabel, limReleaseLabel;

    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using CA = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::unique_ptr<SA> cutoffA, resoA;
    std::unique_ptr<CA> filterTypeA;
    std::unique_ptr<SA> delayTimeA, delayFbA, delayMixA;
    std::unique_ptr<SA> reverbMixA;
    std::unique_ptr<CA> reverbIrA;
    std::unique_ptr<SA> limThreshA, limReleaseA;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EffectsPanel)
};
