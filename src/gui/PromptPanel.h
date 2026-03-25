#pragma once
#include <JuceHeader.h>
#include "WaveformDisplay.h"

class T5ynthProcessor;

/**
 * SOURCE column: 2 prompts with alpha between them, generation controls, waveform.
 */
class PromptPanel : public juce::Component
{
public:
    explicit PromptPanel(T5ynthProcessor& processor);
    ~PromptPanel() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void triggerGeneration();

    T5ynthProcessor& processorRef;

    juce::Label headerLabel;

    // Prompts + interpolation
    juce::TextEditor promptAEditor;
    juce::Slider alphaSlider;       // horizontal between A and B
    juce::TextEditor promptBEditor;

    // Secondary: embedding control
    juce::Slider magnitudeKnob, noiseKnob;
    juce::Label magnitudeLabel, noiseLabel;

    // Tertiary: generation tech settings (compact)
    juce::Slider durationSlider, stepsSlider, cfgSlider;
    juce::Label durationLabel, stepsLabel, cfgLabel;

    juce::TextButton generateButton { "GENERATE" };
    WaveformDisplay waveformDisplay;
    juce::Label infoLabel;

    bool generating = false;

    using Attachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<Attachment> alphaAttach, magnitudeAttach, noiseAttach;
    std::unique_ptr<Attachment> durationAttach, stepsAttach, cfgAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PromptPanel)
};
