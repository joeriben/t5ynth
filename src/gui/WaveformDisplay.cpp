#include "WaveformDisplay.h"

WaveformDisplay::WaveformDisplay()
{
    startTimerHz(30);
}

void WaveformDisplay::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff0a0a0a));

    const juce::ScopedLock lock(dataLock);

    if (waveformData.empty())
        return;

    const auto bounds = getLocalBounds().toFloat().reduced(2.0f);
    const float midY = bounds.getCentreY();
    const float halfHeight = bounds.getHeight() * 0.5f;
    const int numPoints = static_cast<int>(waveformData.size());

    juce::Path path;
    path.startNewSubPath(bounds.getX(),
                         midY - waveformData[0] * halfHeight);

    for (int i = 1; i < numPoints; ++i)
    {
        float x = bounds.getX() + (static_cast<float>(i) / static_cast<float>(numPoints - 1)) * bounds.getWidth();
        float y = midY - waveformData[static_cast<size_t>(i)] * halfHeight;
        path.lineTo(x, y);
    }

    g.setColour(juce::Colour(0xff4a9eff));
    g.strokePath(path, juce::PathStrokeType(1.5f));
}

void WaveformDisplay::resized()
{
}

void WaveformDisplay::setWaveform(const float* data, int numSamples)
{
    const juce::ScopedLock lock(dataLock);
    waveformData.assign(data, data + numSamples);
}

void WaveformDisplay::timerCallback()
{
    repaint();
}
