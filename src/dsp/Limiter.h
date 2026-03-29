#pragma once
#include <JuceHeader.h>

/**
 * Output limiter wrapping juce::dsp::Limiter.
 *
 * Sits at the end of the signal chain to prevent clipping.
 */
class T5ynthLimiter
{
public:
    T5ynthLimiter() = default;

    void prepare(double sampleRate, int samplesPerBlock);
    void processBlock(juce::AudioBuffer<float>& buffer);
    void reset();

    /** Set threshold in dB (default: -3.0 dB). */
    void setThreshold(float dB);

    /** Set release time in ms (default: 100 ms). */
    void setRelease(float ms);

private:
    juce::dsp::Limiter<float> limiter;
    bool prepared = false;
};
