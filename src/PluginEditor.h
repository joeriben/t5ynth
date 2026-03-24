#pragma once
#include <JuceHeader.h>

class T5ynthProcessor;

/**
 * Main editor window for T5ynth.
 *
 * Contains the root GUI layout and manages the LookAndFeel.
 */
class T5ynthEditor : public juce::AudioProcessorEditor
{
public:
    explicit T5ynthEditor(T5ynthProcessor& processor);
    ~T5ynthEditor() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    T5ynthProcessor& processorRef;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(T5ynthEditor)
};
