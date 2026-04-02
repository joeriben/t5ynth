#pragma once
#include <JuceHeader.h>

/**
 * Algorithmic reverb wrapping juce::dsp::Reverb (Freeverb).
 *
 * Same interface as ConvolutionReverb for drop-in use.
 */
class AlgorithmicReverb
{
public:
    AlgorithmicReverb() = default;

    void prepare(double sampleRate, int samplesPerBlock);
    void processBlock(juce::AudioBuffer<float>& buffer);
    void reset();

    /** Set dry/wet mix (0=dry, 1=wet). */
    void setMix(float mix);

private:
    juce::dsp::Reverb reverb;
    juce::dsp::DryWetMixer<float> mixer;
    float wetMix = 0.0f;
    bool prepared = false;

    // Silence detection
    int silentInputBlocks = 0;
    static constexpr int REVERB_TAIL_BLOCKS = 344;
};
