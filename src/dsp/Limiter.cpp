#include "Limiter.h"

void T5ynthLimiter::prepare(double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = 2;

    limiter.prepare(spec);
    limiter.setThreshold(-0.3f);
    limiter.setRelease(100.0f);
    prepared = true;
}

void T5ynthLimiter::processBlock(juce::AudioBuffer<float>& buffer)
{
    if (!prepared)
        return;

    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    limiter.process(context);
}

void T5ynthLimiter::reset()
{
    limiter.reset();
}

void T5ynthLimiter::setThreshold(float dB)
{
    limiter.setThreshold(dB);
}

void T5ynthLimiter::setRelease(float ms)
{
    limiter.setRelease(ms);
}
