#include "PluginEditor.h"
#include "PluginProcessor.h"

T5ynthEditor::T5ynthEditor(T5ynthProcessor& processor)
    : AudioProcessorEditor(processor),
      processorRef(processor)
{
    setSize(1200, 800);
    setResizable(true, true);
    setResizeLimits(800, 600, 2400, 1600);
}

void T5ynthEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff0a0a0a));

    g.setColour(juce::Colour(0xffe3e3e3));
    g.setFont(juce::FontOptions(32.0f));
    g.drawText("T5ynth", getLocalBounds(), juce::Justification::centred);
}

void T5ynthEditor::resized()
{
    // Layout stub: sub-panels will be positioned here
}
