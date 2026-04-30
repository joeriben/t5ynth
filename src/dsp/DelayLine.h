#pragma once
#include <JuceHeader.h>

/**
 * Delay effect.
 *
 * Stereo delay with time, feedback, dry/wet mix, and feedback damping LP filter.
 * Damping: LP filter in feedback loop with exponential mapping
 *   0 = bright (20kHz), 1 = dark (500Hz): freq = 20000 * pow(500/20000, d).
 * Mix is a true crossfade: out = dry*(1-mix) + wet*mix; at mix=1 the dry
 * path vanishes (matches the reverb behaviour).
 */
class T5ynthDelayLine
{
public:
    T5ynthDelayLine() = default;

    void prepare(double sampleRate, int samplesPerBlock);
    void processBlock(juce::AudioBuffer<float>& buffer);
    void reset();

    /** Set delay time in milliseconds (smoothed per-sample). */
    void setTime(float ms);

    /** Set feedback amount (0-0.95, smoothed per-sample). */
    void setFeedback(float fb);

    /** Set dry/wet mix (0=dry, 1=wet). True crossfade: at mix=1 dry vanishes. */
    void setMix(float mix);

    /** Set feedback damping (0=bright 20kHz, 1=dark 500Hz). */
    void setDamp(float d);

    float getMix() const { return wetMix; }

private:
    juce::dsp::DelayLine<float> delayLine { 96000 }; // Max 2 seconds at 48kHz

    // Feedback damping: one LP filter per channel
    juce::dsp::IIR::Filter<float> dampFilterL;
    juce::dsp::IIR::Filter<float> dampFilterR;

    double sr = 44100.0;
    float delayTimeMs = 250.0f;     // Reference default
    float targetDelaySamples = 0.0f; // smoothing target
    float currentDelaySamples = 0.0f; // smoothed current
    float feedback = 0.35f;          // Reference default
    float targetFeedback = 0.35f;    // smoothing target
    float wetMix = 0.3f;            // Reference default (send amount)
    float dampFreq = 4000.0f;       // Default at damp=0.5
    bool prepared = false;

    // Silence detection — skip processing only after output has truly decayed
    int silentOutputBlocks = 0;
    static constexpr int SILENCE_CONFIRM_BLOCKS = 4;

    void updateDampCoeffs();
};
