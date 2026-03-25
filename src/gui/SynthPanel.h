#pragma once
#include <JuceHeader.h>

/**
 * OSC / ENV / MOD column: engine mode, scan, 3 envelopes, 2 LFOs, drift.
 */
class SynthPanel : public juce::Component
{
public:
    explicit SynthPanel(juce::AudioProcessorValueTreeState& apvts);
    ~SynthPanel() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    // Engine
    juce::ComboBox engineModeBox;

    // Oscillator
    juce::Slider scanKnob;
    juce::Label scanLabel;

    // Amp Envelope (4 knobs in a row)
    juce::Slider ampA, ampD, ampS, ampR;
    juce::Label ampAL, ampDL, ampSL, ampRL;

    // Mod Envelope 1
    juce::Slider mod1A, mod1D, mod1S, mod1R;
    juce::Label mod1AL, mod1DL, mod1SL, mod1RL;

    // Mod Envelope 2
    juce::Slider mod2A, mod2D, mod2S, mod2R;
    juce::Label mod2AL, mod2DL, mod2SL, mod2RL;

    // LFO 1
    juce::Slider lfo1Rate, lfo1Depth;
    juce::ComboBox lfo1Wave;
    juce::Label lfo1RateL, lfo1DepthL;

    // LFO 2
    juce::Slider lfo2Rate, lfo2Depth;
    juce::ComboBox lfo2Wave;
    juce::Label lfo2RateL, lfo2DepthL;

    // Drift
    juce::ToggleButton driftToggle { "Drift" };
    juce::Slider drift1Rate, drift1Depth, drift2Rate, drift2Depth, drift3Rate, drift3Depth;

    // Attachments
    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using CA = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<CA> engineModeAttach;
    std::unique_ptr<SA> scanAttach;
    std::unique_ptr<SA> ampAA, ampDA, ampSA, ampRA;
    std::unique_ptr<SA> mod1AA, mod1DA, mod1SA, mod1RA;
    std::unique_ptr<SA> mod2AA, mod2DA, mod2SA, mod2RA;
    std::unique_ptr<SA> lfo1RateA, lfo1DepthA;
    std::unique_ptr<CA> lfo1WaveA;
    std::unique_ptr<SA> lfo2RateA, lfo2DepthA;
    std::unique_ptr<CA> lfo2WaveA;
    std::unique_ptr<BA> driftEnableA;
    std::unique_ptr<SA> d1RA, d1DA, d2RA, d2DA, d3RA, d3DA;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SynthPanel)
};
