#pragma once
#include <JuceHeader.h>

/**
 * Waveform visualization component.
 *
 * Displays the current wavetable frame or looper waveform.
 * Updates at ~30fps using a timer.
 */
class WaveformDisplay : public juce::Component,
                        private juce::Timer
{
public:
    WaveformDisplay();
    ~WaveformDisplay() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    /** Set waveform data to display. */
    void setWaveform(const float* data, int numSamples);

private:
    void timerCallback() override;

    std::vector<float> waveformData;
    juce::CriticalSection dataLock;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveformDisplay)
};
