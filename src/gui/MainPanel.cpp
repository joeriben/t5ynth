#include "MainPanel.h"

MainPanel::MainPanel()
{
}

void MainPanel::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff0a0a0a));
}

void MainPanel::resized()
{
    // Layout stub: sub-panels will be positioned here
}
