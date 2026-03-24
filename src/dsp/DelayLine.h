#pragma once
#include <JuceHeader.h>

/**
 * Delay effect wrapping juce::dsp::DelayLine.
 *
 * Stereo delay with time, feedback, and dry/wet mix.
 */
class T5ynthDelayLine
{
public:
    T5ynthDelayLine() = default;

    void prepare(double sampleRate, int samplesPerBlock);
    void processBlock(juce::AudioBuffer<float>& buffer);
    void reset();

    /** Set delay time in milliseconds. */
    void setTime(float ms);

    /** Set feedback amount (0-0.95). */
    void setFeedback(float fb);

    /** Set dry/wet mix (0=dry, 1=wet). */
    void setMix(float mix);

private:
    juce::dsp::DelayLine<float> delayLine { 220500 }; // Max ~5 seconds at 44.1kHz
    double sr = 44100.0;
    float delayTimeMs = 300.0f;
    float feedback = 0.3f;
    float wetMix = 0.0f;
    bool prepared = false;
};
