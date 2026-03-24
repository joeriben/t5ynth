#include "StatusBar.h"

StatusBar::StatusBar()
{
}

void StatusBar::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1a1a1a));

    g.setColour(backendConnected ? juce::Colour(0xff4ade80) : juce::Colour(0xffef4444));
    g.fillEllipse(8.0f, (getHeight() - 8.0f) * 0.5f, 8.0f, 8.0f);

    g.setColour(juce::Colour(0xffe3e3e3));
    g.setFont(13.0f);
    g.drawText(statusText, 24, 0, getWidth() - 32, getHeight(),
               juce::Justification::centredLeft);
}

void StatusBar::resized()
{
}

void StatusBar::setStatusText(const juce::String& text)
{
    statusText = text;
    repaint();
}

void StatusBar::setConnected(bool connected)
{
    backendConnected = connected;
    repaint();
}
