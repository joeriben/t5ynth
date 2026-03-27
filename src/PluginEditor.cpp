#include "PluginEditor.h"
#include "PluginProcessor.h"

T5ynthEditor::T5ynthEditor(T5ynthProcessor& processor)
    : AudioProcessorEditor(processor),
      processorRef(processor),
      mainPanel(processor)
{
    setLookAndFeel(&lookAndFeel);
    addAndMakeVisible(mainPanel);

    setSize(1200, 800);
    setResizable(true, true);
    setResizeLimits(900, 600, 2400, 1600);
    getConstrainer()->setFixedAspectRatio(1200.0 / 800.0);
}

T5ynthEditor::~T5ynthEditor()
{
    setLookAndFeel(nullptr);
}

void T5ynthEditor::paint(juce::Graphics&)
{
}

void T5ynthEditor::resized()
{
    mainPanel.setBounds(getLocalBounds());
}
